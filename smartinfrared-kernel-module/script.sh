#!/bin/bash

module="smartinfrared.ko"

remove_module() {
    echo "Removing kernel module..."
    sudo rmmod $module 2>/dev/null || echo "Module not loaded."
    echo "Cleaning..."
    make clean
}

dmesg_pid=0

stop_dmesg() {
    if [[ $dmesg_pid -ne 0 ]]; then
        echo "Parando monitoramento de logs do SmartInfrared (PID: $dmesg_pid)..."
        kill $dmesg_pid
        wait $dmesg_pid 2>/dev/null
        dmesg_pid=0
    fi
}

trap "remove_module; stop_dmesg; exit 0" SIGINT

while true; do

    echo "Select an option:"
    echo "1 - Iniciar os processos"
    echo "2 - Executar lsusb -t"
    echo "3 - Remover o módulo e parar monitoramento"
    echo "4 - Monitorar logs do SmartInfrared"
    echo "5 - Sair"

    read -p "Digite a opção desejada: " option

    case $option in
        1)
            make

            echo "Removing kernel module: cp210x"
            sudo rmmod cp210x 2>/dev/null || echo "Module cp210x not loaded."

            echo "Installing kernel module: {$module}" 
            time sudo insmod $module
            ;;

        2)
            echo "Executing lsusb -t"
            lsusb -t
            ;;

        3)
            remove_module
            stop_dmesg
            ;;

        4)
            if [[ $dmesg_pid -eq 0 ]]; then
                echo "Monitorando logs do SmartInfrared em segundo plano..."
                sudo dmesg -w &
                dmesg_pid=$!
            else
                echo "Monitoramento de logs já está em execução."
            fi
            ;;

        5)
            remove_module
            stop_dmesg
            echo "Saindo..."
            sudo modprobe cp210x
            exit 0
            ;;

        *)
            echo "Opção inválida. Tente novamente..."
            ;;
    esac

    echo ""
done
