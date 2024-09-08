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

// Tópico MQTT
const char* topic_publish1 = "XXXX"; // Tópico para envio da informação de movimento

// Definir pinos
const int pirPin1 = 14;  // Pino do sensor PIR 1 d5

WiFiClientSecure espClient;
PubSubClient client(espClient);

// Conexao a wifi
void setup_wifi() {
  delay(10);
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
  pinMode(pirPin1, INPUT);    // Configura o pino do sensor PIR 1 como entrada
  Serial.begin(115200);
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
  
  // Verifica o estado dos sensores PIR
  int pirState1 = digitalRead(pirPin1);
  
  // Publica a mensagem se houver movimento detectado por qualquer sensor
  if (pirState1 == HIGH) {
    Serial.println("Movimento detectado pelo sensor 1!");
    client.publish(topic_publish1, "MOVIMENTO");
  } else {
    client.publish(topic_publish1, "SEM MOVIMENTO");
  }
  }
  
  delay(1000); // Aguarda 1 segundo antes de verificar novamente
}
