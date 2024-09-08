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

// Tópicos MQTT
const char* topic_rele1 = "XXXX"; // Tópico para sinal de uso do relé
const char* topic_rele2 = "XXXX"; // Tópico para sinal de uso do relé
const char* topic_rele3 = "XXXX"; // Tópico para sinal de uso do relé
const char* topic_rele4 = "XXXX"; // Tópico para sinal de uso do relé

// Pinos
const int pino_rele1 = 13;
const int pino_rele2 = 5;
const int pino_rele3 = 4;
const int pino_rele4 = 0;

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

void callback(char* topic, byte* payload, unsigned int length) {
  String message;
  for (unsigned int i = 0; i < length; i++) {
    message += (char)payload[i];
  }

  Serial.print("Mensagem recebida [");
  Serial.print(topic);
  Serial.print("]: ");
  Serial.println(message);
  
  if (String(topic) == topic_rele1) {
    if (message == "ON") {
      digitalWrite(pino_rele2, LOW); // Relé ligado 
    } else if (message == "OFF") {
      digitalWrite(pino_rele2, HIGH); // Relé desligado
    }
  }
  if (String(topic) == topic_rele2) {
    if (message == "ON") {
      digitalWrite(pino_rele2, LOW); // Relé ligado 
    } else if (message == "OFF") {
      digitalWrite(pino_rele2, HIGH); // Relé desligado
    }
  }
  if (String(topic) == topic_rele3) {
    if (message == "ON") {
      digitalWrite(pino_rele3, LOW); // Relé ligado 
    } else if (message == "OFF") {
      digitalWrite(pino_rele3, HIGH); // Relé desligado
    }
  }
  if (String(topic) == topic_rele4) {
    if (message == "ON") {
      digitalWrite(pino_rele4, LOW); // Relé ligado 
    } else if (message == "OFF") {
      digitalWrite(pino_rele4, HIGH); // Relé desligado
    }
  }
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Tentando conectar ao MQTT...");
    if (client.connect("ESP8266Client", mqtt_user, mqtt_password)) {
      Serial.println("conectado");
      client.subscribe(topic_rele1);
      client.subscribe(topic_rele2);
      client.subscribe(topic_rele3);
      client.subscribe(topic_rele4);
    } else {
      Serial.print("falhou, rc=");
      Serial.print(client.state());
      Serial.println(" tentando novamente em 5 segundos");
      delay(5000);
    }
  }
}

void setup() {
  pinMode(pino_rele1, OUTPUT);
  pinMode(pino_rele2, OUTPUT);
  pinMode(pino_rele3, OUTPUT);
  pinMode(pino_rele4, OUTPUT);
  digitalWrite(pino_rele1, HIGH); // Relé desligado inicialmente
  digitalWrite(pino_rele2, HIGH); // Relé desligado inicialmente
  digitalWrite(pino_rele3, HIGH); // Relé desligado inicialmente
  digitalWrite(pino_rele4, HIGH); // Relé desligado inicialmente

  setup_wifi();

  espClient.setInsecure(); // Ignorar certificados SSL/TLS (apenas para testes)
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
}