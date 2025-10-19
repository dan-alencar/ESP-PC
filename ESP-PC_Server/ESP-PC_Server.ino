#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include "AsyncTCP.h"
#define DebugSerial Serial1 // Define nosso "Serial de Debug" como sendo o Serial1

// --- Configurações de Rede ---
const char* ssid = "LESC";     // Coloque o nome da sua rede Wi-Fi aqui
const char* password = "A33669608F"; // Coloque a senha da sua rede aqui

// --- Configurações de Hardware ---
// Pinos GPIO seguros para uso na ESP32-C3
const int POWER_PIN = 4; // Conectado ao pino 1 (Ânodo) do optoacoplador de POWER
const int RESET_PIN = 5; // Conectado ao pino 1 (Ânodo) do optoacoplador de RESET

const int ESP_UART_TX_PIN = 21; // O pino que você especificou
const int ESP_UART_RX_PIN = 20; // Um pino "placeholder", mesmo que não o usemos

// Duração do pulso para simular o "pressionar" do botão
const int PULSE_DURATION_MS = 300;

// --- Configuração do LED de Status (Heartbeat) ---
// Na maioria das placas ESP32-C3, o LED azul onboard está no GPIO 2.
// É um LED "common-anode", então LOW = LIGADO, HIGH = DESLIGADO.
const int LED_PIN = 2;
bool ledState = HIGH; // Começa desligado (HIGH)
unsigned long previousMillis = 0;
const long interval = 1000; // Intervalo do blink (1 segundo)

// --- Objetos do Servidor ---
AsyncWebServer server(80); // Cria o objeto do servidor na porta 80 (HTTP)

// --- Página HTML ---
// (O HTML é idêntico ao da versão anterior)
const char* htmlPage = R"rawliteral(
<!DOCTYPE html>
<html lang="pt-br">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Controlador de PC</title>
    <style>
        body {
            font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, Helvetica, Arial, sans-serif;
            background-color: #121212;
            color: #e0e0e0;
            display: flex;
            flex-direction: column;
            align-items: center;
            justify-content: center;
            height: 100vh;
            margin: 0;
            text-align: center;
        }
        h1 {
            font-weight: 300;
            color: #ffffff;
        }
        .container {
            display: flex;
            flex-direction: column;
            gap: 20px;
            width: 80%;
            max-width: 400px;
        }
        .button {
            display: block;
            padding: 25px 20px;
            font-size: 1.2rem;
            font-weight: bold;
            text-decoration: none;
            color: #ffffff;
            border-radius: 8px;
            transition: transform 0.1s ease, box-shadow 0.2s ease;
            box-shadow: 0 4px 10px rgba(0,0,0,0.4);
        }
        .button:active {
            transform: scale(0.98);
            box-shadow: 0 2px 5px rgba(0,0,0,0.4);
        }
        .power {
            background-color: #4CAF50; /* Verde */
        }
        .power:hover {
            background-color: #45a049;
        }
        .reset {
            background-color: #f44336; /* Vermelho */
        }
        .reset:hover {
            background-color: #e53935;
        }
    </style>
</head>
<body>
    <h1>Controlador de PC</h1>
    <div class="container">
        <a href="/power" class="button power">Ligar / Desligar PC</a>
        <a href="/reset" class="button reset">Resetar PC</a>
    </div>
</body>
</html>
)rawliteral";


void setup() {
    // Adiciona uma pequena espera para a inicialização do Serial USB nativo
    delay(1000); 
    
    Serial.begin(115200);
    Serial.println("\nIniciando Controlador de PC...");

    // NOVO: Inicializa o Serial1 (nosso DebugSerial) nos pinos 21 (TX) e 20 (RX)
    DebugSerial.begin(115200, SERIAL_8N1, ESP_UART_RX_PIN, ESP_UART_TX_PIN);

    DebugSerial.println("\nIniciando Controlador de PC (via UART1)...");

    // --- Configuração dos Pinos ---
    // Pinos de controle do PC
    pinMode(POWER_PIN, OUTPUT);
    pinMode(RESET_PIN, OUTPUT);
    digitalWrite(POWER_PIN, LOW); // Estado inativo
    digitalWrite(RESET_PIN, LOW); // Estado inativo

    // Pino do LED de status
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, HIGH); // Começa desligado (HIGH = OFF)

    // --- Conexão Wi-Fi ---
    WiFi.begin(ssid, password);
    Serial.print("Conectando ao Wi-Fi");
    DebugSerial.println("Conectando ao Wi-Fi"); // Linha nova
    
    // Pisca o LED rapidamente enquanto conecta
    unsigned long connectStartTime = millis();
    while (WiFi.status() != WL_CONNECTED) {
        // Timeout de conexão (ex: 20 segundos)
        if (millis() - connectStartTime > 20000) { 
            Serial.println("\nFalha ao conectar. Reiniciando...");
            DebugSerial.println("\nFalha ao conectar. Reiniciando...");
            ESP.restart();
        }
        
        Serial.print(".");
        DebugSerial.println(".");
        // Blink rápido
        digitalWrite(LED_PIN, LOW); // ON
        delay(100);
        digitalWrite(LED_PIN, HIGH); // OFF
        delay(100);
    }
    
    Serial.println("\nConectado!");
    Serial.print("Endereço IP: ");
    Serial.println(WiFi.localIP());
    DebugSerial.println("\nConectado!");
    DebugSerial.print("Endereço IP: ");
    DebugSerial.println(WiFi.localIP());

    // --- Definição das Rotas do Servidor ---

    // Rota Raiz ("/") - Serve a página HTML principal
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send_P(200, "text/html", htmlPage);
    });

    // Rota "/power" - Aciona o pulso de POWER
    server.on("/power", HTTP_GET, [](AsyncWebServerRequest *request){
        Serial.println("Comando POWER recebido!");
        DebugSerial.println("Comando POWER recebido!");
        
        // Dá um feedback visual IMEDIATO no LED
        digitalWrite(LED_PIN, LOW); // Acende o LED
        
        // Envia o pulso para o PC
        digitalWrite(POWER_PIN, HIGH);
        delay(PULSE_DURATION_MS); // (delay aqui é aceitável, é muito curto)
        digitalWrite(POWER_PIN, LOW);

        // Redireciona o navegador de volta para a página principal
        request->redirect("/");
        
        // O LED voltará ao seu estado normal (piscando) no próximo 'loop'
    });

    // Rota "/reset" - Aciona o pulso de RESET
    server.on("/reset", HTTP_GET, [](AsyncWebServerRequest *request){
        Serial.println("Comando RESET recebido!");
        DebugSerial.println("Comando RESET recebido!");

        // Dá um feedback visual IMEDIATO no LED
        digitalWrite(LED_PIN, LOW); // Acende o LED

        // Envia o pulso para o PC
        digitalWrite(RESET_PIN, HIGH);
        delay(PULSE_DURATION_MS);
        digitalWrite(RESET_PIN, LOW);

        // Redireciona o navegador de volta para a página principal
        request->redirect("/");
        
        // O LED voltará ao seu estado normal (piscando) no próximo 'loop'
    });

    // Rota "Não Encontrado" (404)
    server.onNotFound([](AsyncWebServerRequest *request){
        request->send(404, "text/plain", "Página não encontrada.");
    });

    // Inicia o servidor
    server.begin();
    Serial.println("Servidor Web iniciado. Acesse o IP acima.");
    DebugSerial.println("Servidor Web iniciado. Acesse o IP acima.");
    
    // Deixa o LED aceso fixo por 2 segundos para mostrar que está pronto
    digitalWrite(LED_PIN, LOW); // ON
    delay(2000);
    digitalWrite(LED_PIN, HIGH); // OFF
    previousMillis = millis(); // Reseta o timer do blink para o loop
}

void loop() {
    // Esta é a rotina de "heartbeat" (pulsação)
    // Ela usa millis() para não bloquear o servidor web
    unsigned long currentMillis = millis();

    if (currentMillis - previousMillis >= interval) {
        // Salva o tempo da última vez que o LED piscou
        previousMillis = currentMillis;

        // Inverte o estado do LED
        if (ledState == LOW) {
            ledState = HIGH; // Desliga
        } else {
            ledState = LOW;  // Liga
        }

        // Atualiza o pino do LED
        digitalWrite(LED_PIN, ledState);
    }
    
    // NOTA: Não precisamos de mais nada aqui.
    // O AsyncWebServer cuida de todas as conexões de rede em segundo plano.
}