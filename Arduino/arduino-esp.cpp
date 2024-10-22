#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>

const char* ssid = "armario77";
const char* password = "77armario";
const char* serverName = "http://IP_do_servidor:5000/dados";

void setup() {
  Serial.begin(115200);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Conectando ao WiFi...");
  }
  Serial.println("Conectado ao WiFi");
}

void loop() {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;

    http.begin(serverName);
    http.addHeader("Content-Type", "application/json");

    // Exemplo de JSON enviado
    String jsonData = "{\"sensor_id\": \"1\", \"presence_status\": \"1\"}";

    int httpResponseCode = http.POST(jsonData);

    if (httpResponseCode > 0) {
      String response = http.getString();
      Serial.println(httpResponseCode);
      Serial.println(response);
    } else {
      Serial.print("Erro ao enviar POST: ");
      Serial.println(httpResponseCode);
    }

    http.end();
  }

  delay(10000); // Aguarda 10 segundos antes de enviar novamente
}
