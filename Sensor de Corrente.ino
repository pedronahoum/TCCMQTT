#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <WiFiClientSecure.h>
#include <EmonLib.h> // Inclui a biblioteca EmonLib

/ Configurações da rede Wi-Fi
const char* ssid = "XXXX" // adicionar sua wifi
const char* password = "XXXX"; // adicionar senha da wifi

// Configurações do HiveMQ Cloud
const char* mqtt_server = "XXXX"; // Cluster do seu servidor
const int mqtt_port = XXXX; // Porta utilizada no servidor
const char* mqtt_user = "XXXX"; // Usuário para acesso em servidor, caso tenha.
const char* mqtt_password = "XXXX"; // Senha para acesso em servidor, caso tenha.

// Tópicos
const char* topic_current = "XXXX"; // Tópico para gráfico de consumo
const char* topic_currenttotal = "XXXX"; // Tópico para painel com consumo total

// Define o pino ao qual o sensor de corrente está conectado
float currentPin = A0; // A entrada analógica A0 do ESP8266
EnergyMonitor emon1;
const float tension = 110.0; // Tensão da rede em Volts

// Variáveis para acumular o consumo de energia
double energyConsumed = 0.0; // Energia consumida em Wh
unsigned long previousMillis = 0;
const long interval = 1000; // Intervalo de 1 segundo

WiFiClientSecure espClient; // Usando TLS
PubSubClient client(espClient);

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

  // Inicializa o sensor de corrente
  emon1.current(currentPin, 52.5); // Ajuste o valor de calibração conforme necessário
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;
    
    // Lê a corrente medida pelo sensor
    double Irms = emon1.calcIrms(1480); // Calcula o Irms
    
    // Calcula a potência instantânea em Watts
    double power = Irms * tension;

    // Calcula a energia consumida neste intervalo de 1 segundo (convertendo para horas)
    double energy = power * (interval / 3600000.0); // Wh
    
    // Acumula a energia consumida
    energyConsumed += energy;
    
    // Exibe a corrente e a energia consumida
    Serial.print("Corrente: ");
    Serial.print(Irms);
    Serial.println(" A");

    Serial.print("Energia consumida (W): ");
    Serial.println(energyConsumed);

    // Publica a energia consumida no tópico MQTT
    char messageEnergy[50];
    dtostrf(energyConsumed, 6, 3, messageEnergy);
    client.publish(topic_currenttotal, messageEnergy);

    // Publica a corrente atual no tópico MQTT
    char messageCurrent[50];
    dtostrf(Irms, 6, 3, messageCurrent);
    client.publish(topic_current, messageCurrent);
  }
}
