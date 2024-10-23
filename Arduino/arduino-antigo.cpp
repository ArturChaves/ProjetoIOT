const char* ssid = "rogerio 10s";
const char* password = "roger123";
const char* GScriptId = "AKfycbzlzeuWfDgJyb5ytiroek-mkK2KPVCW8HZ0mAiDWyhHFbrHogeluLRXBFXg6p_FoKkzEA";
String url = String("/macros/s/") + GScriptId + "/exec";
String payload_base = "{\"command\": \"insert_row\", \"sheet_name\": \"Sheet1\", \"values\": ";
String payload = "";

void setup() {
  Serial.begin(115200);
  Wire.begin();
  SPI.begin();
  rfid.PCD_Init();

  // Configuração dos pinos
  pinMode(pinoLedVerde, OUTPUT);
  pinMode(pinoLedVermelho, OUTPUT);
  pinMode(pinoBotao, INPUT_PULLUP);
  for (int i = 0; i < numSensores; i++) {
    pinMode(pinosSensores[i], INPUT);
  }

  // Iniciar comunicação serial com ESP8266
  espSerial.begin(115200); // A maioria dos módulos ESP8266 utiliza 115200 como baud rate

  // Configurar o módulo ESP8266
  connectToWiFi();
}

void connectToWiFi() {
  sendATCommand("AT+RST", 2000); // Reset ESP8266
  sendATCommand("AT+CWMODE=1", 1000); // Modo Cliente
  
  String connectCommand = "AT+CWJAP=\"" + String(ssid) + "\",\"" + String(password) + "\"";
  sendATCommand(connectCommand.c_str(), 10000); // Conectar ao Wi-Fi
  Serial.println("Conectado ao Wi-Fi.");
}

void sendATCommand(const char* command, int timeout) {
  espSerial.println(command);
  long int time = millis();
  while ((time + timeout) > millis()) {
    while (espSerial.available()) {
      char c = espSerial.read();
      Serial.print(c); // Mostra a resposta do ESP8266 no Serial Monitor
    }
  }
}

void leituraRfid() {
  if (!rfid.PICC_IsNewCardPresent() || !rfid.PICC_ReadCardSerial()) {
    return;
  }

  String strID = "";
  for (byte i = 0; i < 4; i++) {
    strID += String(rfid.uid.uidByte[i] < 0x10 ? "0" : "") + String(rfid.uid.uidByte[i], HEX) + (i != 3 ? ":" : "");
  }
  strID.toUpperCase();
  
  if (strID.indexOf("F5:0F:16:AD") >= 0) {
    digitalWrite(pinoLedVerde, HIGH);
    Serial.println("Tag válida detectada: " + strID);
    tagAtiva = strID;
    delay(3000);
    digitalWrite(pinoLedVerde, LOW);
  } else {
    digitalWrite(pinoLedVermelho, HIGH);
    Serial.println("Tag não autorizada: " + strID);
    delay(3000);
    digitalWrite(pinoLedVermelho, LOW);
  }

  rfid.PICC_HaltA();
  rfid.PCD_StopCrypto1();
}

void leituraSensores() {
  if (tagAtiva != "") {
    for (int i = 0; i < numSensores; i++) {
      estadosSensores[i] = digitalRead(pinosSensores[i]);
      Serial.println("Sensor " + String(i) + ": " + (estadosSensores[i] == LOW ? "Obstáculo" : "Sem Obstáculo"));
    }
  } else {
    Serial.println("Nenhuma tag ativa.");
  }
}

void enviarGoogleSheets() {
  payload = payload_base + "\"" + tagAtiva;
  for (int i = 0; i < numSensores; i++) {
    payload += "," + String(estadosSensores[i]);
  }
  payload += "\"}";

  // Constrói a requisição HTTP para o Google Script
  String httpRequest = "AT+CIPSTART=\"TCP\",\"script.google.com\",443"; // Inicia a conexão TCP
  sendATCommand(httpRequest.c_str(), 3000);

  String postRequest = "POST " + url + " HTTP/1.1\r\n" +
                       "Host: script.google.com\r\n" +
                       "Content-Type: application/json\r\n" +
                       "Content-Length: " + String(payload.length()) + "\r\n\r\n" +
                       payload;

  sendATCommand(("AT+CIPSEND=" + String(postRequest.length())).c_str(), 2000);
  sendATCommand(postRequest.c_str(), 2000);

  // Fecha a conexão
  sendATCommand("AT+CIPCLOSE", 1000);
}

void loop() {
  if (digitalRead(pinoBotao) == LOW) {
    leituraSensores();
    enviarGoogleSheets();
    delay(1000);
    tagAtiva = "";
  } else {
    leituraRfid();
  }
  delay(1500);

  return newStateString;
}