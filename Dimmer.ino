#include <Arduino.h>
#include <Ticker.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <WiFiClientSecure.h>

#define PINO_DIM1    14 // D5 no ESP8266
#define PINO_ZC1     12  // D6 no ESP8266

#define maxBrightness 1  // brilho máximo em ms (arredondado)
#define minBrightness 8  // brilho mínimo em ms (arredondado)
#define TRIGGER_TRIAC_INTERVAL 1 // tempo que o triac fica acionado em ms
#define IDLE -1

// Variáveis globais
int brilhoBaixo = 30;  // Inicia com brilho baixo em 30%
int brilhoAlto = 100;  // Brilho alto é sempre 100%
int brilho_convertido1 = 0;
volatile bool isPinHighEnabled1 = false;
volatile long currentBrightness1 = minBrightness;
Ticker timerToPinHigh1, timerToPinLow1;
Ticker returnToLowBrightness; // Ticker para retornar ao brilho baixo após um tempo


// Configurações da rede Wi-Fi
const char* ssid = "XXXX" // adicionar sua wifi
const char* password = "XXXX"; // adicionar senha da wifi

// Configurações do HiveMQ Cloud
const char* mqttServer = "XXXX"; // Cluster do seu servidor
const int mqttPort = XXXX; // Porta utilizada no servidor
const char* mqttUser = "XXXX"; // Usuário para acesso em servidor, caso tenha.
const char* mqttPassword = "XXXX"; // Senha para acesso em servidor, caso tenha.

WiFiClientSecure espClient; // Usando TLS
PubSubClient client(espClient);
const char* topicsensor = "XXXX";  // Substitua pelo seu tópico MQTT
const char* topicbrilho = "XXXX";  // Substitua pelo seu tópico MQTT

void IRAM_ATTR ISR_turnPinLow1() { // Desliga o pino dim
  noInterrupts(); // Desativa interrupções
  digitalWrite(PINO_DIM1, LOW);
  isPinHighEnabled1 = false;
  interrupts(); // Ativa as interrupções novamente
}

void setTimerPinLow1() { // Executa as configurações de PWM e aplica os valores da luminosidade ao dimmer no tempo em que ficará em low
  timerToPinLow1.once_ms(TRIGGER_TRIAC_INTERVAL, ISR_turnPinLow1);
}

void IRAM_ATTR ISR_turnPinHigh1() { // Liga o pino dim
  noInterrupts(); // Desativa interrupções
  digitalWrite(PINO_DIM1, HIGH);
  setTimerPinLow1();
  interrupts(); // Ativa as interrupções novamente
}

void setTimerPinHigh1(long brightness) { // Executa as configurações de PWM e aplica os valores da luminosidade ao dimmer no tempo que ficará em high
  isPinHighEnabled1 = true;
  timerToPinHigh1.once_ms(brightness, ISR_turnPinHigh1);
}

void IRAM_ATTR ISR_zeroCross1() { // Função que é chamada ao dimmer registrar passagem por 0
  if (currentBrightness1 == IDLE) return;
  noInterrupts(); // Desativa interrupções
  if (!isPinHighEnabled1) {
    setTimerPinHigh1(currentBrightness1); // Define o brilho
  }
  interrupts(); // Ativa as interrupções novamente
}

void updateBrightness(int brilho) {
  brilho_convertido1 = map(brilho, 100, 0, maxBrightness, minBrightness);
  noInterrupts(); // Desliga as interrupções
  currentBrightness1 = brilho_convertido1; // Atualiza o brilho
  interrupts(); // Liga as interrupções

  Serial.print("Brilho ajustado para: ");
  Serial.println(brilho); // Mostra o brilho atual
}

void returnToLowBrightnessCallback() {
  // Retorna o brilho para o valor definido como baixo
  updateBrightness(brilhoBaixo);
  Serial.println("Retornando para o brilho baixo.");
}

void mqttCallback(char* topic, byte* payload, unsigned int length) {
  String message;
  for (int i = 0; i < length; i++) {
    message += (char)payload[i];
  }

  // Mensagem no topico de controle do brilho baixo
  if (String(topic) == topicbrilho) {
    int novoBrilhoBaixo = message.toInt(); // Converte a mensagem para número

    // Valida o valor do brilho entre 0 e 100
    if (novoBrilhoBaixo >= 0 && novoBrilhoBaixo <= 100) {
      brilhoBaixo = novoBrilhoBaixo; // Atualiza o brilho baixo
      Serial.print("Novo valor de brilho baixo definido: ");
      Serial.println(brilhoBaixo);
      updateBrightness(brilhoBaixo); // Envia novo brilho para a lampada
    } else {
      Serial.println("Valor de brilho baixo inválido.");
    }
  }

  // Mensagem no topico de controle de brilho alto x baixo
  if (String(topic) == topicsensor) {
    if (message.equals("MOVIMENTO")) {
      Serial.println("'MOVIMENTO' detectado. Ajustando brilho para 100%.");

      // Define o brilho para 100%
      updateBrightness(brilhoAlto);

      // Após 10 segundos (10000 ms), retorna para o brilho baixo
      returnToLowBrightness.once(10, returnToLowBrightnessCallback);
    }
  }
}

void setupWifi() {
  delay(10);
  // Conectando ao WiFi
  Serial.println();
  Serial.print("Conectando-se a ");
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

void setupMQTT() {
  client.setServer(mqttServer, mqttPort);
  client.setCallback(mqttCallback);

  while (!client.connected()) {
    Serial.print("Conectando ao MQTT...");
    if (client.connect("ESP8266Client", mqttUser, mqttPassword)) {
      Serial.println("Conectado!");
      client.subscribe(topicsensor);  // Inscreve no tópico de controle
      client.subscribe(topicbrilho);  // Inscreve no tópico de brilho baixo
    } else {
      Serial.print("Falha ao conectar. Erro: ");
      Serial.print(client.state());
      delay(2000);
    }
  }
}

void setup() {
  Serial.begin(115200); // Inicia a serial

  currentBrightness1 = IDLE;

  pinMode(PINO_ZC1, INPUT_PULLUP);
  pinMode(PINO_DIM1, OUTPUT);
  digitalWrite(PINO_DIM1, LOW);
  attachInterrupt(digitalPinToInterrupt(PINO_ZC1), ISR_zeroCross1, RISING);
  Serial.println("Controlando dimmer com ESP8266");

  // Configura o brilho inicial para o valor baixo
  updateBrightness(brilhoBaixo);

  setupWifi();  // Configura a conexão WiFi
  espClient.setInsecure();
  setupMQTT();  // Configura a conexão MQTT
}

void loop() {
  client.loop();  // Mantém a conexão MQTT
}

