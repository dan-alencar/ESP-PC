/*
 * Ponte Serial (Serial Bridge)
 * para Arduino Nano 33 IoT
 * * Escuta a porta Serial1 (Hardware, Pino 0/RX)
 * e repassa tudo para a Serial (USB / Serial Monitor).
 */

void setup() {
  // Inicializa o Serial Monitor do PC
  Serial.begin(115200);

  // Espera o Serial Monitor conectar (opcional, mas bom para o Nano 33)
  while (!Serial);
  
  Serial.println("--- Monitor Arduino Nano 33 IoT Pronto ---");
  Serial.println("Escutando a ESP32 no Pino RX (Serial1) a 115200 baud...");

  // Inicializa a porta serial de HARDWARE (Pinos RX/TX)
  // que está conectada à ESP32
  Serial1.begin(115200);
}

void loop() {
  // Se a ESP enviou alguma coisa pela Serial1...
  if (Serial1.available()) {
    // ...leia o que ela disse e imprima no monitor do PC (Serial).
    Serial.write(Serial1.read());
  }

  // (Não precisamos fazer mais nada)
}