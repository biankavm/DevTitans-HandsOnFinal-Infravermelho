#include <IRremote.h>

#define IR_SEND_PIN 4  // Pino de envio IR
#define BUTTON_PIN 3   // Pino de um botão para trocar de protocolo
int currentProtocol = 0; // Índice inicial no array de protocolos

IRsend irsend;

struct IRCommand {
    const char* protocol;
    unsigned long code;
    int bits;
};

const IRCommand powerCommands[] = {
    {"NEC",  0xFFA25D, 32}, // Código NEC POWER
    {"Sony", 0xA90,    12}, // Código Sony POWER
    {"RC5",  0x1FE,    14}, // Código RC5 POWER
    {"RC6",  0xD02,    20}, // Código RC6 POWER
    {"Panasonic", 0x400401, 48} // Código Panasonic POWER
};

void setup() {
    Serial.begin(115200);
    Serial.println("Emissor IR configurado");
    pinMode(BUTTON_PIN, INPUT_PULLUP); // Configura botão com pull-up
    irsend.begin();
}

void loop() {
    // Verifica se o botão foi pressionado para trocar de protocolo
    if (digitalRead(BUTTON_PIN) == LOW) {
        currentProtocol = (currentProtocol + 1) % 5; // Alterna entre 0 e 4
        Serial.print("Protocolo selecionado: ");
        Serial.println(powerCommands[currentProtocol].protocol);
        delay(500); // Anti-bounce para o botão
    }

    // Envia o comando "POWER" do protocolo atual
    IRCommand command = powerCommands[currentProtocol];
    if (strcmp(command.protocol, "NEC") == 0) {
        irsend.sendNEC(command.code, command.bits);
    } else if (strcmp(command.protocol, "Sony") == 0) {
        irsend.sendSony(command.code, command.bits);
    } else if (strcmp(command.protocol, "RC5") == 0) {
        irsend.sendRC5(command.code, command.bits);
    } else if (strcmp(command.protocol, "RC6") == 0) {
        irsend.sendRC6(command.code, command.bits);
    } else if (strcmp(command.protocol, "Panasonic") == 0) {
        irsend.sendPanasonic(command.code, command.bits);
    }

    delay(1000); // Espera 1 segundo antes de enviar novamente
}
