#include <WiFi.h>
#include <HTTPClient.h>
#define CURRENT_SENSOR_PIN 34
int relePin = 21;
double Irms = 0;
int indice = 0;

#include "EmonLib.h"
EnergyMonitor emon1;

// Definindo o número de amostras para a média
#define SAMPLES 1000

const char* ssid = "CE342_Esp.Maker";
const char* password = "ce3se4si2_m@ker";

void setup() {

  pinMode(relePin, OUTPUT);

  Serial.begin(115200);
  emon1.current(34, 14.8); //       pino / calibração

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Conectando ao WiFi...");
  }

  Serial.println("Conectado ao WiFi");
  
}

void post(double data){
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;

    String strg = String(data, 2);

    String url = "http://192.168.100.76:7122/api/Leitura/RegistrarLeitura?dispositivoId=1&watts="+strg;
    
    http.begin(url); // Inicia a conexão HTTP
    http.addHeader("Content-Type", "application/x-www-form-urlencoded"); // Define o tipo de conteúdo

    int httpResponseCode = http.POST(""); // Faz a requisição POST com corpo vazio

    if (httpResponseCode > 0) {
      String response = http.getString(); // Obtenha a resposta se necessário
      //Serial.println(httpResponseCode);   // Código de resposta HTTP
      Serial.println(response);           // Resposta do servidor
    } else {
      Serial.print("Erro na requisição: ");
      Serial.println(httpResponseCode);
    }

    http.end(); // Fecha a conexão
  }
}

void pegaValor(){

  for(int i = 0; i < 10; i++){
        Irms = emon1.calcIrms(740);

        Irms -= 0.135;
  }

  if(Irms < 0 || Irms < 0.05){
    Irms = 0;
  }

  Serial.print("\nAmperes: ");
  Serial.println(Irms);
  
  Serial.print("WATTS: ");
  Serial.println(Irms * 110);

  //post(Irms * 110);
}

void loop() {

  pegaValor();
  
  //post(Irms * 110);

}
