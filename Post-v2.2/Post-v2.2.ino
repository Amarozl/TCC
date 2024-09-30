#include <WiFi.h>
#include <HTTPClient.h>
#include "EmonLib.h"

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

// Variáveis para controle de tempo
unsigned long lastMeasurementTime = 0;
const unsigned long measurementInterval = 1000; // 1 segundo

unsigned long lastPostTime = 0;
const unsigned long postInterval = 3600; // 1 hora (3600 segundos)

void setup() {

  Serial.begin(115200);

  emon1.current(34, 13.5); // Inicializa o monitor de corrente

  Irms = emon1.calcIrms(4096);
  Irms = emon1.calcIrms(4096);
  Irms -= 0.40; // Ajuste de calibração

  if (Irms < 0 || Irms <= 0.06) {
    Irms = 0;
  }

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

    String url = "http://192.168.100.76:7122/api/Auth/login";
    http.begin(url);
    http.addHeader("Content-Type", "application/json");

    String jsonPayload = "{\"email\":\"" + email + "\", \"senha\":\"" + senha + "\"}";
    int httpResponseCode = http.POST(jsonPayload);

    if (httpResponseCode > 0) {
      String response = http.getString();
      Serial.println(response);

      // Extraindo o token da resposta
      int tokenIndex = response.indexOf("token");
      if (tokenIndex >= 0) {
        int startIndex = response.indexOf(':', tokenIndex) + 2;
        int endIndex = response.indexOf('"', startIndex);
        jwtToken = response.substring(startIndex, endIndex);
        Serial.println("Token JWT: " + jwtToken);
      }
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