#include <Wire.h>
#include <SPI.h>
#include <MFRC522.h>
#include <SoftwareSerial.h>

#define SS_PIN 53
#define RST_PIN 9
#define pinoBotao 5
const int pinoLedVerde = 3;
const int pinoLedVermelho = 4;
const int numSensores = 3;
int pinosSensores[numSensores] = {38, 36, 34};
int estadosSensores[numSensores];

MFRC522 rfid(SS_PIN, RST_PIN);
SoftwareSerial espSerial(18, 19); // RX1=18 (TX ESP01), TX1=19 (RX ESP01)

String tagAtiva = "";
bool emModoSensor = false;
bool leiturasEnviadas = false;

const char* ssid = "armario07"; // Substitua pelo seu SSID
const char* password = "armario07"; // Substitua pela sua senha Wi-Fi
String server = "bug-free-fortnight-7gp4q77w4w526p9-5000.app.github.dev";  // Endereço da API
String endpoint = "/dados";  // Endpoint da API

void setup() {
  Serial.begin(9600);
  espSerial.begin(115200); // ESP-01 usa 115200 por padrão
  Wire.begin();
  SPI.begin();
  rfid.PCD_Init();
  
  pinMode(pinoLedVerde, OUTPUT);
  pinMode(pinoLedVermelho, OUTPUT);
  pinMode(pinoBotao, INPUT_PULLUP);

  for (int i = 0; i < numSensores; i++) {
    pinMode(pinosSensores[i], INPUT);
  }

  digitalWrite(pinoLedVerde, LOW);
  digitalWrite(pinoLedVermelho, LOW);

  conectarWiFi();

  Serial.println("Sistema iniciado. Aguardando leitura RFID...");
}

void conectarWiFi() {
  espSerial.println("AT+CWMODE=1"); // Modo cliente
  delay(1000);

  String cmd = "AT+CWJAP=\"" + String(ssid) + "\",\"" + String(password) + "\"";
  espSerial.println(cmd); // Conectar ao Wi-Fi
  delay(5000);

  if (espSerial.find("OK")) {
    Serial.println("Conectado ao Wi-Fi!");
  } else {
    Serial.println("Falha ao conectar no Wi-Fi");
  }
}

void enviarDadosAPI(int sensor_id, bool presence_status) {
  String jsonData = "{\"sensor_id\": \"" + String(sensor_id) + "\", \"presence_status\": " + (presence_status ? "true" : "false") + "}";
  
  espSerial.println("AT+CIPSTART=\"TCP\",\"" + server + "\",80"); // Conectar ao servidor
  delay(2000);

  if (espSerial.find("OK")) {
    String postRequest = "POST " + endpoint + " HTTP/1.1\r\n";
    postRequest += "Host: " + server + "\r\n";
    postRequest += "Content-Type: application/json\r\n";
    postRequest += "Content-Length: " + String(jsonData.length()) + "\r\n\r\n";
    postRequest += jsonData;

    espSerial.println("AT+CIPSEND=" + String(postRequest.length()));
    delay(1000);

    if (espSerial.find(">")) {
      espSerial.print(postRequest);
      delay(2000);
      
      if (espSerial.find("SEND OK")) {
        Serial.println("Dados enviados com sucesso!");
      } else {
        Serial.println("Erro ao enviar os dados.");
      }
    } else {
      Serial.println("Erro ao iniciar envio.");
    }
  } else {
    Serial.println("Erro ao conectar ao servidor.");
  }

  espSerial.println("AT+CIPCLOSE"); // Fechar conexão
  delay(1000);
}

void leituraRfid() {
  if (!rfid.PICC_IsNewCardPresent() || !rfid.PICC_ReadCardSerial()) {
    return;
  }

  String strID = "";
  for (byte i = 0; i < 4; i++) {
    strID += (rfid.uid.uidByte[i] < 0x10 ? "0" : "") +
             String(rfid.uid.uidByte[i], HEX) +
             (i != 3 ? ":" : "");
  }
  strID.toUpperCase();

  if (strID.indexOf("F5:0F:16:AD") >= 0) {  // Exemplo de uma tag autorizada
    digitalWrite(pinoLedVerde, HIGH);
    Serial.print("Tag válida detectada: ");
    Serial.println(strID);
    tagAtiva = strID;
    emModoSensor = true;
    leiturasEnviadas = false;
    delay(3000);
    digitalWrite(pinoLedVerde, LOW);
  } else {
    digitalWrite(pinoLedVermelho, HIGH);
    Serial.print("Tag não autorizada: ");
    Serial.println(strID);
    delay(3000);
    digitalWrite(pinoLedVermelho, LOW);
  }

  rfid.PICC_HaltA();
  rfid.PCD_StopCrypto1();
}

void leituraSensorObstaculos(int sensores[], int estados[], int num) {
  if (tagAtiva != "") {
    for (int i = 0; i < num; i++) {
      estados[i] = digitalRead(sensores[i]);
      bool presence_status = (estados[i] == LOW);
      enviarDadosAPI(i, presence_status);
      
      if (presence_status) {
        Serial.print("Obstáculo detectado no sensor ");
      } else {
        Serial.print("Nenhum obstáculo detectado no sensor ");
      }
      Serial.print(i);
      Serial.print(" pela tag: ");
      Serial.println(tagAtiva);
    }
    leiturasEnviadas = true;
  } else {
    Serial.println("Nenhuma tag válida detectada.");
  }
}

void loop() {
  if (digitalRead(pinoBotao) == LOW) {
    if (emModoSensor) {
      Serial.println("Botão pressionado: lendo sensores novamente antes de resetar...");
      leituraSensorObstaculos(pinosSensores, estadosSensores, numSensores);
      delay(500);
    }

    emModoSensor = false;
    tagAtiva = "";
    leiturasEnviadas = false;
    Serial.println("Modo de leitura RFID ativado. Aguardando nova tag...");
    delay(1000);
  }

  if (emModoSensor) {
    if (!leiturasEnviadas) {
      Serial.println("Lendo sensores de obstáculos...");
      leituraSensorObstaculos(pinosSensores, estadosSensores, numSensores);
    }
  } else {
    Serial.println("Aguardando leitura RFID...");
    leituraRfid();
  }

  delay(1500);
}
