#include <linux/module.h>
#include <linux/usb.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/kthread.h>
#include <linux/delay.h>
#include <linux/uaccess.h>

#define VENDOR_ID   0x10c4
#define PRODUCT_ID  0xea60

#define CP210X_IFC_ENABLE     0x00
#define CP210X_SET_BAUDRATE   0x1E
#define CP210X_SET_LINE_CTL   0x03
#define CP210X_SET_MHS        0x07

#define REQTYPE_HOST_TO_INTERFACE (USB_TYPE_VENDOR | USB_RECIP_INTERFACE | USB_DIR_OUT)

#define UART_ENABLE       0x0001
#define UART_DISABLE      0x0000

#define CONTROL_DTR       0x0001
#define CONTROL_RTS       0x0002
#define CONTROL_DTR_RTS   (CONTROL_DTR | CONTROL_RTS)

#define BITS_DATA_8       0x0800  

#define MAX_RECV_LINE 800

static struct usb_device *smartinfrared_device;
static uint usb_in, usb_out;
static char *usb_in_buffer, *usb_out_buffer;
static int usb_max_size;

#define VENDOR_ID   0x10c4
#define PRODUCT_ID  0xea60
static const struct usb_device_id id_table[] = { { USB_DEVICE(VENDOR_ID, PRODUCT_ID) }, {} };

static int usb_probe(struct usb_interface *ifce, const struct usb_device_id *id);
static void usb_disconnect(struct usb_interface *ifce);
static int usb_write_serial(const char *cmd);
static int usb_read_serial(char *response, size_t size);
static int set_baud_rate(struct usb_device *udev, uint32_t baud_rate);
static int cp210x_ifc_enable(struct usb_device *udev, u16 ifnum);
static int cp210x_set_line_ctl(struct usb_device *udev, u16 ifnum, u16 line_ctl);
static int cp210x_set_mhs(struct usb_device *udev, u16 ifnum, u16 control);
static int cp210x_set_baudrate(struct usb_device *udev, u16 ifnum, u32 baud);


static ssize_t attr_show(struct kobject *sys_obj, struct kobj_attribute *attr, char *buff);
static ssize_t attr_store(struct kobject *sys_obj, struct kobj_attribute *attr, const char *buff, size_t count);

static struct kobj_attribute pattern_attribute = __ATTR(pattern, 0660, attr_show, attr_store);
static struct attribute *attrs[] = { &pattern_attribute.attr, NULL };
static struct attribute_group attr_group = { .attrs = attrs };
static struct kobject *sys_obj;

MODULE_DEVICE_TABLE(usb, id_table);

static struct usb_driver smartinfrared_driver = {
    .name        = "smartinfrared",
    .probe       = usb_probe,
    .disconnect  = usb_disconnect,
    .id_table    = id_table,
};



module_usb_driver(smartinfrared_driver);



MODULE_AUTHOR("DevTITANS <devtitans@icomp.ufam.edu.br>");
MODULE_DESCRIPTION("Driver de acesso ao SmartInfrared (ESP32 com Chip Serial CP2102");
MODULE_LICENSE("GPL");




static int usb_probe(struct usb_interface *interface, const struct usb_device_id *id) {
    struct usb_endpoint_descriptor *usb_endpoint_in, *usb_endpoint_out;
    int ret;
    u16 ifnum;
    printk(KERN_INFO "SmartInfrared: Dispositivo conectado ...\n");
    
    sys_obj = kobject_create_and_add("smartInfrared", kernel_kobj);
    if (!sys_obj || sysfs_create_group(sys_obj, &attr_group)) {
        kobject_put(sys_obj);
        printk(KERN_ERR "SmartInfrared: Falha ao criar arquivos no SysFs\n");
        return -ENOMEM;
    }

    smartinfrared_device = interface_to_usbdev(interface);

    ifnum = interface->cur_altsetting->desc.bInterfaceNumber;

    if (usb_find_common_endpoints(interface->cur_altsetting,
                                  &usb_endpoint_in, &usb_endpoint_out,
                                  NULL, NULL)) {
        printk(KERN_ERR "SmartInfrared: Falha ao localizar endpoints USB\n");
        return -ENODEV;
    }

    usb_in = usb_endpoint_in->bEndpointAddress;
    usb_out = usb_endpoint_out->bEndpointAddress;
    usb_max_size = usb_endpoint_maxp(usb_endpoint_in);

    usb_in_buffer = kmalloc(usb_max_size, GFP_KERNEL);
    usb_out_buffer = kmalloc(usb_max_size, GFP_KERNEL);
    if (!usb_in_buffer || !usb_out_buffer) {
        printk(KERN_ERR "SmartInfrared: Falha ao alocar buffers\n");
        return -ENOMEM;
    }

   
    ret = cp210x_ifc_enable(smartinfrared_device, ifnum);
    if (ret < 0) {
        printk(KERN_ERR "SmartInfrared: cp210x_ifc_enable falhou: %d\n", ret);
        goto error;
    }

    ret = cp210x_set_line_ctl(smartinfrared_device, ifnum, BITS_DATA_8 /*0x0800*/);
    if (ret < 0) {
        printk(KERN_ERR "SmartInfrared: set_line_ctl falhou: %d\n", ret);
        goto error;
    }

    ret = cp210x_set_baudrate(smartinfrared_device, ifnum, 115200);
    if (ret < 0) {
        printk(KERN_ERR "SmartInfrared: set_baudrate falhou: %d\n", ret);
        goto error;
    }

    ret = cp210x_set_mhs(smartinfrared_device, ifnum, CONTROL_DTR_RTS);
    if (ret < 0) {
        printk(KERN_ERR "SmartInfrared: set_mhs falhou: %d\n", ret);
        goto error;
    }

    printk(KERN_INFO "SmartInfrared: CP2102 UART configurado (8N1, baud=115200, DTR/RTS).\n");

    return 0;

    error:
    kfree(usb_in_buffer);
    kfree(usb_out_buffer);
    return ret;
}


static void usb_disconnect(struct usb_interface *interface) {
    printk(KERN_INFO "SmartInfrared: Dispositivo desconectado.\n");

    

    if (sys_obj)
        kobject_put(sys_obj);


    kfree(usb_in_buffer);
    kfree(usb_out_buffer);
}

static int usb_write_serial(const char *cmd) {
    int ret, actual_size;
    printk(KERN_INFO "SmartInfrared: Enviando comando: %s\n", cmd);

    snprintf(usb_out_buffer, usb_max_size, "%s", cmd);
    ret = usb_bulk_msg(smartinfrared_device, usb_sndbulkpipe(smartinfrared_device, usb_out),
                       usb_out_buffer, strlen(usb_out_buffer), &actual_size, 1000);
    if (ret) {
        printk(KERN_ERR "SmartInfrared: Falha ao enviar comando. Código: %d\n", ret);
    }
    return ret;
}


static int usb_read_serial(char *response, size_t size) {
    int ret, actual_size;
    int attempts = 0;
    char full_response[MAX_RECV_LINE] = {0};
    char *newline_ptr = NULL, *start_ptr = NULL;

    while (attempts < 30) {
        ret = usb_bulk_msg(smartinfrared_device, usb_rcvbulkpipe(smartinfrared_device, usb_in),
                           usb_in_buffer, usb_max_size, &actual_size, 3000);
        if (ret) {
            printk(KERN_ERR "SmartInfrared: Falha ao ler resposta. Código: %d\n", ret);
            return -1;
        }

        usb_in_buffer[actual_size] = '\0';
        strncat(full_response, usb_in_buffer, sizeof(full_response) - strlen(full_response) - 1);

        printk(KERN_INFO "SmartInfrared: Dados recebidos: %s\n", usb_in_buffer);
        printk(KERN_INFO "SmartInfrared: Dados acumulados: %s\n", full_response);

        if (!start_ptr) {
            start_ptr = strstr(full_response, "RECV_");
            if (!start_ptr) {
                start_ptr = strstr(full_response, "SEND_");
            }

            if (!start_ptr) {
                attempts++;
                printk(KERN_INFO "SmartInfrared: Nenhuma resposta válida encontrada. Tentativa: %d\n", attempts);
                continue; 
            }
        }

        newline_ptr = strchr(start_ptr, '\n');
        if (newline_ptr) {
            *newline_ptr = '\0'; 
            snprintf(response, size, "%s", start_ptr);
            return 0; 
        }

        printk(KERN_INFO "SmartInfrared: Resposta parcial acumulada: %s\n", full_response);
    }

    printk(KERN_ERR "SmartInfrared: Falha ao ler resposta após várias tentativas.\n");
    return -1;
}



static ssize_t attr_show(struct kobject *sys_obj, struct kobj_attribute *attr, char *buff) {
    char response[MAX_RECV_LINE]; 
    if (usb_write_serial("RECV") < 0 || usb_read_serial(response, sizeof(response)) < 0) {
        return -EIO;
    }
    return scnprintf(buff, PAGE_SIZE, "%s\n", response);
}

static ssize_t attr_store(struct kobject *sys_obj, struct kobj_attribute *attr, const char *buff, size_t count)
{
    char cmd[MAX_RECV_LINE];
    char response[MAX_RECV_LINE];

    if (strncmp(buff, "LAST", 4) == 0) {
        snprintf(cmd, sizeof(cmd), "LAST");
    }
    else {
        
        snprintf(cmd, sizeof(cmd), "SEND_%s", buff);
    }

    if (usb_write_serial(cmd) < 0) {
        return -EIO;
    }
    if (usb_read_serial(response, sizeof(response)) < 0) {
        return -EIO;
    }

    printk(KERN_INFO "SmartInfrared: Resposta do dispositivo: %s\n", response);
    
    return count;
}

static int cp210x_ifc_enable(struct usb_device *udev, u16 ifnum)
{
    return usb_control_msg(udev,
                           usb_sndctrlpipe(udev, 0),
                           CP210X_IFC_ENABLE,
                           REQTYPE_HOST_TO_INTERFACE,
                           UART_ENABLE,     
                           ifnum,           
                           NULL, 0,
                           1000);
}

static int cp210x_set_line_ctl(struct usb_device *udev, u16 ifnum, u16 line_ctl)
{
    return usb_control_msg(udev,
                           usb_sndctrlpipe(udev, 0),
                           CP210X_SET_LINE_CTL,
                           REQTYPE_HOST_TO_INTERFACE,
                           line_ctl,
                           ifnum,
                           NULL, 0,
                           1000);
}

static int cp210x_set_mhs(struct usb_device *udev, u16 ifnum, u16 control)
{
    return usb_control_msg(udev,
                           usb_sndctrlpipe(udev, 0),
                           CP210X_SET_MHS,
                           REQTYPE_HOST_TO_INTERFACE,
                           control,
                           ifnum,
                           NULL, 0,
                           1000);
}


static int cp210x_set_baudrate(struct usb_device *udev, u16 ifnum, u32 baud)
{
    __le32 rate = cpu_to_le32(baud);
    int ret;

    ret = usb_control_msg(udev,
                          usb_sndctrlpipe(udev, 0),
                          CP210X_SET_BAUDRATE,
                          REQTYPE_HOST_TO_INTERFACE,
                          0,               
                          ifnum,           
                          &rate,           
                          sizeof(rate),
                          1000);
    return ret;
}