#include <IRremote.h>

#define PIN_RECEIVER 15
#define IR_SEND_PIN 18

IRrecv receiver(PIN_RECEIVER);
//IrSenderClass IrSender;

uint16_t* rawbuf = nullptr;  // Ponteiro para armazenar os dados do sinal IR
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

void loop() {
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
    
    if (Serial.available()) {
        String command = Serial.readStringUntil('\n');
        command.trim();
        
        if (command == "RECV" && lastSignal.length > 0) {
            Serial.print("RECV_freq,");
            for (uint16_t i = 0; i < lastSignal.length; i++) {
                Serial.print(lastSignal.data[i]);
                if (i < lastSignal.length - 1) {
                    Serial.print(",");
                }
            }
            Serial.println();
        }
        else if (command.startsWith("SEND_freq,")) {
            Serial.println("Enviando sinal recebido via Serial...");
            
            std::vector<uint16_t> rawValues;
            char *token = strtok(const_cast<char*>(command.c_str()) + 9, ","); // Pular "SEND_freq,"
            while (token != nullptr) {
                rawValues.push_back(atoi(token));
                token = strtok(nullptr, ",");
            }
            
            if (!rawValues.empty()) {
                IrSender.sendRaw(rawValues.data(), rawValues.size(), 38);
            }
            Serial.println("SEND_OK");
            
        } else if (command == "SEND_freq" && lastSignal.length > 0) {
            Serial.println("Enviando último sinal recebido...");
            IrSender.sendRaw(lastSignal.data, lastSignal.length, 38); // 38 kHz é a frequência comum para sinais IR
        }
    }
}
