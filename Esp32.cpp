#include <IRRemoteESP32.h>
#include <driver/ledc.h>

#define RECV_PIN 19    // GPIO para receber sinais IR
#define SEND_PIN 18     // GPIO para enviar sinais IR
#define BUTTON_PIN 23   // GPIO do botão para envio
#define CARRIER_FREQ 38000 // Frequência de 38kHz (padrão IR)

IRRemoteESP32 irRemote(RECV_PIN);

// Variável para armazenar o código IR recebido
int lastCode = -1;

void setup() {
    Serial.begin(115200);

    // Inicializa o receptor IR
    irRemote = IRRemoteESP32(RECV_PIN);
    pinMode(BUTTON_PIN, INPUT_PULLUP); // Configura o botão como entrada com pull-up

    // Configura o GPIO de envio
    ledcSetup(0, CARRIER_FREQ, 10); // Canal 0, 38kHz, resolução de 10 bits
    ledcAttachPin(SEND_PIN, 0);

    Serial.println("Configuração concluída.");
}

void loop() {
    // Verifica se foi recebido um código IR
    int result = irRemote.checkRemote();
    if (result != -1) {
        lastCode = result; // Armazena o último código recebido
        Serial.printf("Código IR recebido: %d\n", lastCode);
    }

    // Verifica se o botão foi pressionado
    if (digitalRead(BUTTON_PIN) == LOW) {
        if (lastCode != -1) {
            Serial.println("Enviando código IR...");
            sendIRSignal(lastCode);
        } else {
            Serial.println("Nenhum código IR armazenado para enviar.");
        }
        delay(500); // Anti-bounce para o botão
    }
}

// Função para enviar o sinal IR armazenado
void sendIRSignal(int code) {
    for (int i = 0; i < 32; i++) { // Supondo 32 bits para um código NEC
        if (code & (1 << (31 - i))) {
            // Envia "1" como pulso ON seguido de OFF
            ledcWrite(0, 512); // Pulso ON (50% duty cycle)
            delayMicroseconds(560); // Marca
            ledcWrite(0, 0);        // Pulso OFF
            delayMicroseconds(1690); // Espaço
        } else {
            // Envia "0" como pulso ON seguido de OFF
            ledcWrite(0, 512); // Pulso ON (50% duty cycle)
            delayMicroseconds(560); // Marca
            ledcWrite(0, 0);        // Pulso OFF
            delayMicroseconds(560);  // Espaço
        }
    }
    // Encerra o envio com um sinal OFF
    ledcWrite(0, 0);
}
