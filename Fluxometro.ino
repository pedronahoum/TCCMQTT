#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <WiFiClientSecure.h>

// Configurações da rede Wi-Fi
const char* ssid = "XXXX" // adicionar sua wifi
const char* password = "XXXX"; // adicionar senha da wifi

// Configurações do HiveMQ Cloud
const char* mqtt_server = "XXXX"; // Cluster do seu servidor
const int mqtt_port = XXXX; // Porta utilizada no servidor
const char* mqtt_user = "XXXX"; // Usuário para acesso em servidor, caso tenha.
const char* mqtt_password = "XXXX"; // Senha para acesso em servidor, caso tenha.

const char* mqtt_fluxoatual = "XXXX"; // Tópico para gráfico de consumo
const char* mqtt_fluxototal = "XXXX"; // Tópico para painel com consumo total

// Pino do sensor de fluxo
const int flowSensorPin = 13;

// Variáveis para medir o fluxo
volatile int pulseCount = 0;
unsigned long lastTime = 0;
float flowRate = 0.0;
float totalLiters = 0.0;

WiFiClientSecure espClient; // Usando TLS
PubSubClient client(espClient);

void IRAM_ATTR pulseCounter() {
  pulseCount++;
}

void setup_wifi() {
  delay(10);
  Serial.begin(115200);
  Serial.println();
  Serial.print("Conectando a ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi conectado");
  Serial.println("Endereço IP: ");
  Serial.println(WiFi.localIP());
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Tentando conectar ao MQTT...");
    if (client.connect("ESP8266Client", mqtt_user, mqtt_password)) {
      Serial.println("conectado");
    } else {
      Serial.print("falhou, rc=");
      Serial.print(client.state());
      Serial.println(" tentando novamente em 5 segundos");
      delay(5000);
    }
  }
}

void setup() {
  Serial.begin(115200);
  Serial.println("Iniciando...");

  setup_wifi();
  espClient.setInsecure();
  client.setServer(mqtt_server, mqtt_port);
  
  pinMode(flowSensorPin, INPUT);
  attachInterrupt(digitalPinToInterrupt(flowSensorPin), pulseCounter, FALLING);
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  unsigned long currentTime = millis();
  unsigned long elapsedTime = currentTime - lastTime;

  if (elapsedTime > 1000) {  // Calcula a cada segundo
    detachInterrupt(digitalPinToInterrupt(flowSensorPin));
    
    // Calcular a taxa de fluxo em L/min
    // YF-S201: 1 pulso = 2.25 mL
    // Fluxo (L/min) = (pulsos / segundos) * 60 / (pulsos por litro)
    flowRate = (pulseCount / (float)elapsedTime) * 60000.0 / 450.0; // L por Minuto
    
    // Calcular o volume total em litros
    totalLiters += (pulseCount * 2.222) / 1000.0; // Conversão direta de pulsos para litros
    
    Serial.print("RATE: ");
    Serial.println(flowRate);
    Serial.print("TOTAL: ");
    Serial.println(totalLiters);
    
    // Resetar a contagem de pulsos
    pulseCount = 0;
    lastTime = currentTime;
    
    attachInterrupt(digitalPinToInterrupt(flowSensorPin), pulseCounter, FALLING);
    
    // Converter valores para string e publicar via MQTT
    char fluxoAtualStr[8];
    dtostrf(flowRate, 1, 2, fluxoAtualStr);
    client.publish(mqtt_fluxoatual, fluxoAtualStr);
    
    char fluxoTotalStr[8];
    dtostrf(totalLiters, 1, 2, fluxoTotalStr);
    client.publish(mqtt_fluxototal, fluxoTotalStr);
  }
}