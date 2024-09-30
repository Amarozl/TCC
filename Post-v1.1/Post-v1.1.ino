#include <WiFi.h>
#include <HTTPClient.h>
#define CURRENT_SENSOR_PIN 34
int relePin = 21;
float Irms = 0;
float calcMedIrms = 0;
int indice = 0;
float valtotal;

#include "EmonLib.h"
EnergyMonitor emon1;

#define SAMPLES 1000  // Definindo o número de amostras para a média

const char* ssid = "Rede sem fio gratis";
const char* password = "18116843869c";

// Variáveis para o timer
unsigned long previousMillis = 0;  // Armazena o último tempo em que a função foi chamada
const unsigned long interval = 3600000;  // 1 hora em milissegundos (3600000 ms)

// Variáveis para medição
unsigned long previousMeasureMillis = 0;
const unsigned long measureInterval = 1000;  // 1 segundo para medir a corrente

void setup() {

  pinMode(relePin, OUTPUT);

  Serial.begin(115200);
  //emon1.current(34, 14.8)
  emon1.current(34, 13.6); //       pino / calibração

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Conectando ao WiFi...");
  }

  Serial.println("Conectado ao WiFi");
}

void post(float data) {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;

    String strg = String(data, 14);

    String url = "http://192.168.100.76:7122/api/Leitura/RegistrarLeitura?dispositivoId=1&watts=" + strg;
    
    http.begin(url); // Inicia a conexão HTTP
    http.addHeader("Content-Type", "application/x-www-form-urlencoded"); // Define o tipo de conteúdo

    int httpResponseCode = http.POST(""); // Faz a requisição POST com corpo vazio

    if (httpResponseCode > 0) {
      String response = http.getString(); // Obtenha a resposta se necessário
      Serial.println(response);           // Resposta do servidor
    } else {
      Serial.print("Erro na requisição: ");
      Serial.println(httpResponseCode);
    }

    http.end(); // Fecha a conexão
  }
}

void pegaValor() {
  for (int i = 0; i < 10; i++) {
    Irms = emon1.calcIrms(1480);
    Serial.print(Irms);
    Irms -= 0.42; // Ajuste de calibração para remover offset
    Serial.print(" | ");
    Serial.println(Irms);
  }


  if (Irms < 0 || Irms <= 0.06) {
    Irms = 0;
  }

  Serial.print("\nAmperes: ");
  Serial.println(Irms);

  Serial.print("WATTS: ");
  Serial.println(Irms * 110);
  valtotal += (Irms * 110) / 3600000; // Acumula watts convertidos para kWh

  Serial.println(valtotal, 14);
}

void loop() {
  unsigned long currentMillis = millis();

  // Verifica se 1 segundo se passou para medir a corrente
  if (currentMillis - previousMeasureMillis >= measureInterval) {
    previousMeasureMillis = currentMillis;
    pegaValor(); // Chama a função de medição de corrente a cada segundo
  }

  // Verifica se 1 hora se passou para enviar os dados
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;
    post(valtotal); // Envia os dados acumulados após 1 hora
    valtotal = 0; // Reinicia o acumulador após enviar
  }
}
