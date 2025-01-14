#include <IRremote.h>

int RECV_PIN = 18;  // Pino de recepção do IR no ESP32 (pode ser alterado)
int BUTTON_PIN = 19;  // Pino do botão (pode ser alterado)

IRrecv irrecv(RECV_PIN);
IRsend irsend;

decode_results results;

void setup() {
  Serial.begin(115200);  // Aumentar a velocidade da porta serial no ESP32
  irrecv.enableIRIn(); // Inicia o receptor
  pinMode(BUTTON_PIN, INPUT);
}

// Armazenamento do código recebido
int codeType = -1; // O tipo de código
unsigned long codeValue; // O valor do código, se não for raw
unsigned int rawCodes[RAWBUF]; // As durações, se for raw
int codeLen; // O comprimento do código
int toggle = 0; // Estado de alternância RC5/6

// Armazenar o código para reprodução posterior
void storeCode(decode_results *results) {
  codeType = results->decode_type;
  int count = results->rawlen;
  if (codeType == UNKNOWN) {
    Serial.println("Received unknown code, saving as raw");
    codeLen = results->rawlen - 1;
    // Armazenando códigos raw:
    // Ignorar o primeiro valor (gap)
    // Converter de ticks para microssegundos
    // Ajustar os marks e spaces para compensar a distorção do receptor IR
    for (int i = 1; i <= codeLen; i++) {       
      if (i % 2) {         // Mark
        rawCodes[i - 1] = results->rawbuf[i] * USECPERTICK - MARK_EXCESS;
        Serial.print(" m");
      } 
      else {  // Space
        rawCodes[i - 1] = results->rawbuf[i] * USECPERTICK + MARK_EXCESS;
        Serial.print(" s");
      }
      Serial.print(rawCodes[i - 1], DEC);
    }
    Serial.println("");
  }
  else {
    if (codeType == NEC) {
      Serial.print("Received NEC: ");
      if (results->value == REPEAT) {
        // Não gravar um valor de repetição NEC
        Serial.println("repeat; ignoring.");
        return;
      }
    } 
    else if (codeType == SONY) {
      Serial.print("Received SONY: ");
    } 
    else if (codeType == RC5) {
      Serial.print("Received RC5: ");
    } 
    else if (codeType == RC6) {
      Serial.print("Received RC6: ");
    } 
    else {
      Serial.print("Unexpected codeType ");
      Serial.print(codeType, DEC);
      Serial.println("");
    }
    Serial.println(results->value, HEX);
    codeValue = results->value;
    codeLen = results->bits;
  }
}

void sendCode(int repeat) {
  if (codeType == NEC) {
    if (repeat) {
      irsend.sendNEC(REPEAT, codeLen);
      Serial.println("Sent NEC repeat");
    } 
    else {
      irsend.sendNEC(codeValue, codeLen);
      Serial.print("Sent NEC ");
      Serial.println(codeValue, HEX);
    }
  } 
  else if (codeType == SONY) {
    irsend.sendSony(codeValue, codeLen);
    Serial.print("Sent Sony ");
    Serial.println(codeValue, HEX);
  } 
  else if (codeType == RC5 || codeType == RC6) {
    if (!repeat) {
      // Alterna o bit de toggle para uma nova pressão de botão
      toggle = 1 - toggle;
    }
    // Coloca o bit de toggle no código a ser enviado
    codeValue = codeValue & ~(1 << (codeLen - 1));
    codeValue = codeValue | (toggle << (codeLen - 1));
    if (codeType == RC5) {
      Serial.print("Sent RC5 ");
      Serial.println(codeValue, HEX);
      irsend.sendRC5(codeValue, codeLen);
    } 
    else {
      irsend.sendRC6(codeValue, codeLen);
      Serial.print("Sent RC6 ");
      Serial.println(codeValue, HEX);
    }
  } 
  else {
    // Assume 38 KHz
    irsend.sendRaw(rawCodes, codeLen, 38);
    Serial.println("Sent raw");
  }
}

int lastButtonState;

void loop() {
  // Se o botão for pressionado, enviar o código.
  int buttonState = digitalRead(BUTTON_PIN);
  if (lastButtonState == HIGH && buttonState == LOW) {
    Serial.println("Released");
    irrecv.enableIRIn(); // Reabilitar receptor
  }

  if (buttonState) {
    Serial.println("Pressed, sending");    
    sendCode(lastButtonState == buttonState);
    delay(50); // Aguardar um pouco entre as retransmissões
  } 
  else if (irrecv.decode(&results)) {
    storeCode(&results);
    irrecv.resume(); // Retomar o receptor
  }
  lastButtonState = buttonState;
}
