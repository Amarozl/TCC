#include <WiFi.h>
#include <HTTPClient.h>
#include "EmonLib.h"

#define CURRENT_SENSOR_PIN 34
int relePin = 21;
float Irms = 0;
float valtotal;

EnergyMonitor emon1;

const char* ssid = "Rede sem fio gratis";
const char* password = "18116843869c";

// Configurações para login
const String email = "luccasamaroec2024@gmail.com"; // Substitua pelo seu email
const String senha = "1234"; // Substitua pela sua senha
String jwtToken = ""; // Variável para armazenar o token

void setup() {
  pinMode(relePin, OUTPUT);
  Serial.begin(115200);
  
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

    // URL para a requisição de login
    String url = "http://192.168.100.76:7122/api/Auth/login";
    
    // Configura a requisição
    http.begin(url);
    http.addHeader("Content-Type", "application/json"); // Define o tipo de conteúdo

    // Cria o JSON com email e senha
    String jsonPayload = "{\"email\":\"" + email + "\", \"senha\":\"" + senha + "\"}";

    // Faz a requisição POST
    int httpResponseCode = http.POST(jsonPayload);

    if (httpResponseCode > 0) {
      String response = http.getString(); // Obtém a resposta
      Serial.println(response); // Exibe a resposta

      // Supondo que a resposta contém o token em um campo "token"
      // Você pode precisar ajustar isso conforme a estrutura da sua resposta
      int tokenIndex = response.indexOf("token");
      if (tokenIndex >= 0) {
        int startIndex = response.indexOf(':', tokenIndex) + 2; // +2 para pular ": "
        int endIndex = response.indexOf('"', startIndex);
        jwtToken = response.substring(startIndex, endIndex); // Armazena o token
        Serial.println("Token JWT: " + jwtToken);
      }
    } else {
      Serial.print("Erro na requisição: ");
      Serial.println(httpResponseCode);
    }

    http.end(); // Fecha a conexão
  }
}

void post(float data) {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;

    String strg = String(data, 14);
    String url = "http://192.168.100.76:7122/api/Leitura/RegistrarLeitura?dispositivoId=1&watts=" + strg;

    http.begin(url);
    http.addHeader("Content-Type", "application/x-www-form-urlencoded");
    http.addHeader("Authorization", "Bearer " + jwtToken); // Adiciona o cabeçalho Authorization

    int httpResponseCode = http.POST("");
    if (httpResponseCode > 0) {
      String response = http.getString();
      Serial.println(response);
    } else {
      Serial.print("Erro na requisição: ");
      Serial.println(httpResponseCode);
    }

    Serial.println(httpResponseCode);

    http.end();
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
  // Chame suas funções de medição e envio de dados
  pegaValor();
  post(valtotal);
}