#include <IRremote.h>
#define PIN_RECEIVER 2   // Define o pino onde o receptor infravermelho está conectado

IRrecv receiver(PIN_RECEIVER); // Cria um objeto IRrecv associado ao pino do receptor

void setup()
{
  Serial.begin(115200); // Inicializa a comunicação serial a 115200 bauds
  Serial.println("<Pressione um botão>"); // Exibe uma mensagem no monitor serial
  receiver.enableIRIn(); // Inicializa o receptor infravermelho para receber sinais infravermelhos
}

void loop()
{
  if (receiver.decode()) { // Verifica se um sinal infravermelho foi decodificado
    Serial.print(receiver.decodedIRData.command);// Exibe o código do comando infravermelho
    receiver.resume(); // Reinicia o receptor infravermelho para receber mais comandos
  }
}
