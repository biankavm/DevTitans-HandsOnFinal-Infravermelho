#include <Arduino.h>
#include <IRremote.hpp>

// Definindo o pino de envio como o pino 4
#define IR_SEND_PIN 4

// Raw data fornecido (em microssegundos)
const uint16_t rawbuf[] = {
  9000, 4400, 600, 1650, 600, 1600, 600, 1650, 600, 500, 650, 450, 600, 550, 600, 500, 600, 1650, 600, 500, 
  600, 1650, 600, 1600, 600, 1650, 600, 1600, 600, 500, 650, 1600, 600, 500, 650, 450, 650, 1650, 550, 500, 
  650, 500, 600, 1650, 600, 450, 650, 500, 600, 500, 600, 1650, 600, 500, 600, 1650, 600, 1650, 600, 450, 
  650, 1650, 550, 1650, 600, 1650, 600
};

void setup() {
    // Inicializando o pino de envio IR
    pinMode(IR_SEND_PIN, OUTPUT);

    // Inicializando o monitor serial
    Serial.begin(115200);
    Serial.println(F("Iniciando envio de sinal IR"));

    IrSender.begin(IR_SEND_PIN); // Iniciar o envio no pino definido
}

void loop() {
    Serial.println(F("Enviando sinal IR personalizado"));

    // Enviando o sinal com base no rawbuf
    IrSender.sendRaw(rawbuf, sizeof(rawbuf) / sizeof(rawbuf[0]), 38); // 38 kHz é a frequência comum para sinais IR

    delay(1000); // Atraso entre os envios
}
