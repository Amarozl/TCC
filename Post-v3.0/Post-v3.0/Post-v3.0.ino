#include <WiFi.h>
#include <HTTPClient.h>
#include "EmonLib.h"
#include <ArduinoJson.h>

#define CURRENT_SENSOR_PIN 34

float Irms = 0;
float valtotal;

EnergyMonitor emon1;

// Informações da rede WiFi
const char* ssid = "Rede sem fio gratis";
const char* password = "18116843869c";

// Configurações para login
const String email = "luccasamaroec2024@gmail.com"; // Substitua pelo seu email
const String senha = "1234"; // Substitua pela sua senha
String jwtToken = ""; // Variável para armazenar o token
unsigned long tokenExpiryTime = 1 * 60 * 60 * 1000; // Armazena o tempo de expiração do token em milissegundos (1h)

// Variáveis para controle de tempo
unsigned long lastMeasurementTime = 0;
const unsigned long measurementInterval = 1000; // 1 segundo

unsigned long lastPostTime = 0;
const unsigned long postInterval = 1000; // 1 hora (em milissegundos)

// Renova o token se restarem menos de 5 minutos
const unsigned long tokenRenewalThreshold = 10 * 60 * 1000; // 5 minutos

void setup() {
  Serial.begin(115200);
  emon1.current(CURRENT_SENSOR_PIN, 13.5); // Inicializa o monitor de corrente

  // Conectar ao Wi-Fi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Conectando ao WiFi...");
  }
  Serial.println("Conectado ao WiFi");

  // Chama a função para fazer login e obter o token
  login();
}

void login() {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;

    String url = "http://192.168.100.76:7122/api/Auth/login?Email=" + email + "&Senha=" + senha;
    http.begin(url);
    http.addHeader("Content-Type", "application/x-www-form-urlencoded"); // Pode ser necessário ajustar se sua API espera outro tipo

    int httpResponseCode = http.POST(""); // O corpo da requisição pode ser vazio, pois estamos enviando dados pela URL

    if (httpResponseCode > 0) {
      String response = http.getString();
      Serial.println(response);

      // Analise a resposta JSON para extrair o token
      DynamicJsonDocument doc(1024);
      deserializeJson(doc, response);

      // Extraindo o token da resposta
      jwtToken = doc["token"].as<String>();
      Serial.println("Token JWT: " + jwtToken);

      // Verifique a expiração do token se necessário
      unsigned long expiresIn = doc["expires_in"].as<unsigned long>() * 1000;
      tokenExpiryTime = millis() + expiresIn;

    } else {
      Serial.print("Erro na requisição: ");
      Serial.println(httpResponseCode);
    }

    http.end();
  }
}

void post(float data) {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;

    String strg = String(data, 14);
    String url = "http://192.168.100.76:7122/api/Leitura/RegistrarLeitura?dispositivoId=1&watts=" + strg;

    http.begin(url);
    http.addHeader("Content-Type", "application/x-www-form-urlencoded");
    http.addHeader("Authorization", "Bearer " + jwtToken);

    int httpResponseCode = http.POST("");
    if (httpResponseCode > 0) {
      String response = http.getString();
      Serial.println(response);
    } else {
      Serial.print("Erro na requisição: ");
      Serial.println(httpResponseCode);
    }

    http.end();
  }
}

void pegaValor() {
  Irms = emon1.calcIrms(4096);
  Serial.println("\n----------------------");
  Serial.print(Irms);
  Irms -= 0.40; // Ajuste de calibração
  Serial.print(" | ");
  Serial.println(Irms);
  Serial.println("----------------------");

  if (Irms < 0 || Irms <= 0.06) {
    Irms = 0;
  }

  Serial.print(Irms);
  Serial.print("mA");
  Serial.print(" | ");
  Serial.print("WATTS: ");
  Serial.println(Irms * 110);
  Serial.println("----------------------");
  valtotal += (Irms * 110) / 3600000; // Acumula watts convertidos para kWh

  Serial.println(valtotal, 14);
  Serial.println("----------------------");
}

void loop() {
  unsigned long currentMillis = millis();

  // Verifica se o token está próximo de expirar e faz login para renovar
  if (tokenExpiryTime - currentMillis <= tokenRenewalThreshold) {
    Serial.println("Renovando token...");
    login(); // Renova o token antes de expirar
  }

  // Verifica se passou 1 segundo para a medição
  if (currentMillis - lastMeasurementTime >= measurementInterval) {
    lastMeasurementTime = currentMillis;

    // Chame a função de medição
    pegaValor();
  }

  // Verifica se passou 1 hora para o envio
  if (currentMillis - lastPostTime >= postInterval) {
    lastPostTime = currentMillis;

    // Faz o envio do valor total acumulado na última hora
    post(valtotal);

    // Zera o valor total após o envio
    valtotal = 0;
  }
}