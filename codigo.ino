#include <Wire.h> 
#include <LiquidCrystal_I2C.h>
#include <WiFi.h>          
#include <PubSubClient.h>      

// --- Configuração do WiFi ---
const char* ssid = "Wokwi-GUEST";
const char* password = ""; 

// --- Configuração do MQTT ---
const char* mqtt_server = "20.97.192.88";  // Professor troque pelo seu servidor
const char* mqtt_topic = "fiap/gs/postura";    

WiFiClient espClient;
PubSubClient client(espClient);

// --- Componentes do Projeto ---
LiquidCrystal_I2C LCD = LiquidCrystal_I2C(0x27, 16, 2);
int ledVermelho = 25;
int ledVerde = 26;
int buzina = 5;
int pinoTrig = 14;
int pinoEcho = 15;
long duracao;
float distancia;
const int DISTANCIA_RUIM = 30; 
const int DISTANCIA_BOA = 45; 

unsigned long tempoLeituraAnterior = 0;
const long intervaloLeitura = 2000; 

// --- Função para conectar ao WiFi ---
void setup_wifi() {
  delay(10);
  Serial.println();
  Serial.print("Conectando em ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi conectado!");
  Serial.println("IP: ");
  Serial.println(WiFi.localIP());
}

// --- Função para reconectar ao MQTT ---
void reconnect_mqtt() {
  while (!client.connected()) {
    Serial.print("Conectando ao MQTT...");
    if (client.connect("ESP32Client_Postura")) {
      Serial.println("conectado!");
    } else {
      Serial.print("falhou, rc=");
      Serial.print(client.state());
      Serial.println(" Tente novamente em 2 segundos");
      delay(2000); 
    }
  }
}

void setup() {
  pinMode(ledVermelho, OUTPUT);
  pinMode(ledVerde, OUTPUT);
  pinMode(buzina, OUTPUT);
  pinMode(pinoTrig, OUTPUT);
  pinMode(pinoEcho, INPUT); 
  Serial.begin(9600);
  LCD.init();
  LCD.backlight();
  LCD.setCursor(0, 0);
  LCD.print("Guar. Postura");
  digitalWrite(ledVerde, HIGH);
  delay(1000);
  setup_wifi();
  client.setServer(mqtt_server, 1883);
}

float lerDistancia() {
  digitalWrite(pinoTrig, LOW);
  delayMicroseconds(2);
  digitalWrite(pinoTrig, HIGH);
  delayMicroseconds(10);
  digitalWrite(pinoTrig, LOW);
  duracao = pulseIn(pinoEcho, HIGH);
  distancia = (duracao * 0.0343) / 2;
  return distancia;
}

// --- LOOP()  ---
void loop() {
  
  // 1. Garante que o MQTT está conectado 
  if (!client.connected()) {
    reconnect_mqtt();
  }
  // 2. Permite que o MQTT envie/receba
  client.loop(); 

  // 3. Pega o tempo atual
  unsigned long tempoAtual = millis();

  // 4. Lógica de tempo
  if (tempoAtual - tempoLeituraAnterior >= intervaloLeitura) {
    
    // 5. Salva o tempo da última leitura
    tempoLeituraAnterior = tempoAtual;
    
    float dist = lerDistancia();
    Serial.print("Distancia: ");
    Serial.print(dist);
    Serial.println(" cm");

    if (dist <= DISTANCIA_RUIM) {
      // --- POSTURA RUIM ---
      digitalWrite(ledVerde, LOW);
      digitalWrite(ledVermelho, HIGH);
      digitalWrite(buzina, HIGH);
      delay(100); 
      digitalWrite(buzina, LOW);
      
      LCD.clear();
      LCD.setCursor(0, 0);
      LCD.print("POSTURA RUIM!");
      LCD.setCursor(0, 1);
      LCD.print("Corrija-se...");

      // --- ENVIA MENSAGEM MQTT (RUIM) ---
      String payload = "{\"status\": \"ruim\", \"distancia_cm\": " + String(dist) + "}";
      Serial.println("Enviando MQTT: " + payload);
      client.publish(mqtt_topic, payload.c_str());

    } else if (dist > DISTANCIA_RUIM && dist <= DISTANCIA_BOA) {
      // --- POSTURA OK ---
      digitalWrite(ledVerde, HIGH);
      digitalWrite(ledVermelho, LOW);
      digitalWrite(buzina, LOW);
      LCD.clear();
      LCD.setCursor(0, 0);
      LCD.print("Postura OK!");
      LCD.setCursor(0, 1);
      LCD.print(dist);
      LCD.print(" cm");

      // --- ENVIA MENSAGEM MQTT (OK) ---
      String payload = "{\"status\": \"ok\", \"distancia_cm\": " + String(dist) + "}";
      Serial.println("Enviando MQTT: " + payload);
      client.publish(mqtt_topic, payload.c_str());

    } else {
      // --- MUITO LONGE (Usuário saiu) ---
      digitalWrite(ledVerde, LOW);
      digitalWrite(ledVermelho, LOW);
      digitalWrite(buzina, LOW);
      LCD.clear();
      LCD.setCursor(0, 0);
      LCD.print("Guar. Postura");
      LCD.setCursor(0, 1);
      LCD.print("Modo de espera");
      
      // --- ENVIA MENSAGEM MQTT (AUSENTE) ---
      String payload = "{\"status\": \"ausente\", \"distancia_cm\": " + String(dist) + "}";
      Serial.println("Enviando MQTT: " + payload);
      client.publish(mqtt_topic, payload.c_str());
    }
  }
}