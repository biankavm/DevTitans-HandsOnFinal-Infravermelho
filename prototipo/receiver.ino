#include <IRremote.h>
#define PIN_RECEIVER 2

IRrecv receiver(PIN_RECEIVER);

void setup() {
  Serial.begin(115200);
  Serial.println("Aguardando sinal IR...");
  receiver.enableIRIn();
}

void loop() {
  if (receiver.decode()) {
    //Serial.print(receiver.decodedIRData.command);
    Serial.print("RECV_freq");
    //Serial.print(receiver.decodedIRData.frequency);
    Serial.print(",");

    // Exibe os valores raw como uma lista separada por vírgulas
    for (uint16_t i = 1; i < receiver.decodedIRData.rawDataPtr->rawlen; i++) {
      Serial.print(receiver.decodedIRData.rawDataPtr->rawbuf[i] * USECPERTICK);
      if (i < receiver.decodedIRData.rawDataPtr->rawlen - 1) {
        Serial.print(","); // Adiciona vírgula se não for o último valor
      }
    }

    Serial.println();
    receiver.resume(); // Pronto para receber outro sinal
  }
}
