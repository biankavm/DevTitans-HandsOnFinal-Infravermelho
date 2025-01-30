#include <IRremote.h>

#define PIN_RECEIVER 15
#define IR_SEND_PIN 18

IRrecv receiver(PIN_RECEIVER);
uint16_t* rawbuf = nullptr;
typedef struct {
    uint16_t* data;
    uint16_t length;
} IRSignal;

IRSignal lastSignal = {nullptr, 0};

void setup() {
    Serial.begin(115200);
    Serial.println("Aguardando sinal IR...");
    receiver.enableIRIn();
    IrSender.begin(IR_SEND_PIN);
}

uint8_t getProtocolFrequency(decode_type_t protocol) {
    if (protocol == NEC) return NEC_KHZ;
    if (protocol == SONY) return SONY_KHZ;
    if (protocol == BOSEWAVE) return BOSEWAVE_KHZ;
    if (protocol == DENON) return DENON_KHZ;
    if (protocol == JVC) return JVC_KHZ;
    if (protocol == LG) return LG_KHZ;
    if (protocol == SAMSUNG) return SAMSUNG_KHZ;
    if (protocol == KASEIKYO) return KASEIKYO_KHZ;
    if (protocol == RC5 || protocol == RC6) return RC5_RC6_KHZ;
    //if (protocol == BEO) return BEO_KHZ;
    return 38; // Default
}

void receiveIRSignal() {
    if (receiver.decode()) {
        if (lastSignal.data) {
            free(lastSignal.data);
        }
        lastSignal.length = receiver.decodedIRData.rawDataPtr->rawlen - 1;
        lastSignal.data = (uint16_t*)malloc(lastSignal.length * sizeof(uint16_t));

        for (uint16_t i = 1; i < receiver.decodedIRData.rawDataPtr->rawlen; i++) {
            lastSignal.data[i - 1] = receiver.decodedIRData.rawDataPtr->rawbuf[i] * USECPERTICK;
        }
        receiver.resume();
    }
}

void processSerialCommand() {
    if (Serial.available()) {
        String command = Serial.readStringUntil('\n');
        command.trim();
        
        if (command == "RECV" && lastSignal.length > 0) {
            uint8_t freq = getProtocolFrequency(receiver.decodedIRData.protocol);
            Serial.print("RECV_");
            Serial.print(freq);
            Serial.print(",");
            for (uint16_t i = 0; i < lastSignal.length; i++) {
                Serial.print(lastSignal.data[i]);
                if (i < lastSignal.length - 1) {
                    Serial.print(",");
                }
            }
            Serial.println();
        }
        else if (command.startsWith("SEND_")) {
            //Serial.println(command.substring(5, command.indexOf(",")));
            int freq = command.substring(5, command.indexOf(",")).toInt();
            Serial.println("Enviando sinal recebido via Serial...");
            
            std::vector<uint16_t> rawValues;
            char *token = strtok(const_cast<char*>(command.c_str()) + command.indexOf(",") + 1, ",");
            while (token != nullptr) {
                rawValues.push_back(atoi(token));
                token = strtok(nullptr, ",");
            }
            
            if (!rawValues.empty()) {
              Serial.println(freq);
              IrSender.sendRaw(rawValues.data(), rawValues.size(), freq);
            }
            Serial.println("SEND_OK");
            
        } else if (command == "LAST" && lastSignal.length > 0) {
            Serial.println("Enviando último sinal recebido...");
            IrSender.sendRaw(lastSignal.data, lastSignal.length, 38);
        }
    }
}

void loop() {
    receiveIRSignal();
    processSerialCommand();
}
