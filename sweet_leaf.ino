#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <uri/UriBraces.h>
#include <uri/UriRegex.h>
#include <Adafruit_Sensor.h>
#include <ArduinoJson.h>
#include <DHT.h>
#include <DHT_U.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include "OutputDevice.h"
#include "WiFiConfigManager.h"
#include "SystemStatus.h"  

#define DHTPIN D4
#define DHTTYPE    DHT22     // DHT 22 (AM2302)

/**
 * AccessPoint * Nome da rede e senha
 */
const char* apSSID = "Sweet Leaf";
const char* apPassword = "42002420";

ESP8266WebServer server(80);
DHT dht(DHTPIN, DHTTYPE);
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", -3 * 3600, 60000); // Fuso horário -3 (Brasília)
LiquidCrystal_I2C lcd(0x27, 16, 2);

WiFiConfigManager wifiConfigManager(apSSID, apPassword, server);
SystemStatus& _sl = SystemStatus::getInstance();


////// CALLBACKS DOS DISPOSITIVOS ////// 
/**
 * SENSOR DHT (Temperatura e Umidade)
 * Cada leitura leva por volta de 250ms 
 * As leituras podem estar até 2 segundos atrasadas (sensor lento)
 */
void onSensorCheck(int currentIndex) {
  float h = dht.readHumidity();
  float t = dht.readTemperature();

  if (isnan(h) || isnan(t)) {
    Serial.println(F("erro 4201: Falha na leitura do sensor!")); // @todo - Implementar modo tela de erro no lcd
    return;
  }  else {
    if (t >= 26.0) { 
      digitalWrite(D5, HIGH); //@todo - Instalar ventilador
    }
    _sl.data.temperature = t;
    _sl.data.humidity = h;
    return;
  }
}

/**
 * Display LCD
 * Pinagem lcd i2c * Vcc = Vin * gnd = gnd * sda = D3 * scl = D2 * Endereço 0x27 * Ajustar o contraste se não estiver visível
 * @todo implementar modo tela de erro 
 */
void onLcdChange(int currentIndex) {
  lcd.clear();
  switch(currentIndex) {
    case 0: 
      lcd.setCursor(0, 0);
      lcd.print("  TEMPERATURA:   ");
      lcd.setCursor(0,1);
      lcd.print("     " + String(_sl.data.temperature) + "C     ");
      break;
    case 1: 
      lcd.setCursor(0, 0);
      lcd.print("    UMIDADE:    ");
      lcd.setCursor(0,1);
      lcd.print("     " + String(_sl.data.humidity) + "%     ");
      break;
    case 2: 
      lcd.setCursor(0, 0);
      lcd.print("     MODO:      ");
      lcd.setCursor(0,1);
      lcd.print("   VEGETATIVO   ");
      break;
  }  
}

/**
 * LÂMPADA PRINCIPAL
 */
void onLightToggle(int currentIndex) {
  bool ledOn = currentIndex % 2 == 0;
  digitalWrite(D0, ledOn ? HIGH : LOW); 
}

/**
 * PROVISÓRIO: SISTEMA DE CIRCULAÇÃO DO AR
 * enquanto o sistema não é instalado, apenas acender um led
 */
void onFan2Toggle(int currentIndex) {
  bool ledOn = currentIndex % 2 == 0;
  digitalWrite(D7, ledOn ? HIGH : LOW); 
}


///// INSTANCIAS DO GERENCIADOR DE DISPOSITIVOS ///// 

/**
 * LÂMPADA
 * Cada modo de operação possúi sua instância
 * startAtMidnight = true, para que os horários da lâmpada sejam humanamente previsíveis
 * x * 60 * 60 = quantidade de segundos em x horas
 */
unsigned long lightIntervalVeg[] = {8 * 60 * 60, 16 * 60 * 60};
unsigned long lightIntervalFlo[] = {12 * 60 * 60, 12 * 60 * 60};

OutputDevice lightVeg(lightIntervalVeg, 2, true, onLightToggle, &timeClient);
OutputDevice lightFlo(lightIntervalFlo, 2, true, onLightToggle, &timeClient);

/**
 * VENTILAÇÃO 1
 * O ventilador será acionado cada vez que o sensor dht ler uma temperatura >= 26°
 * por meio do callback onSensorCheck
 */
unsigned long fan1Interval[] = {30}; // tempo de atualização do sensor: 30 segundos
OutputDevice fan1(fan1Interval, 1, false, onSensorCheck, &timeClient);

/**
 * VENTILAÇÃO 2 
 * @todo - implementar modo vegetativo e de floração para esse dispositivo
 */
unsigned long fan2Interval[] = {60 * 5, 30}; // troca de ar à cada 5 minutos por 30 segundos
OutputDevice fan2(fan2Interval, 2, false, onFan2Toggle, &timeClient);

/**
 * DISPLAY LCD
 */
unsigned long lcdIntervals[] = {5, 3, 3};
OutputDevice lcdDisplay(lcdIntervals, 3, false, onLcdChange, &timeClient);


///// CALLBACKS DO SERVIDOR WEB //////

/**
 * DADOS DO SISTEMA
 * { temperature: float, humidity: float, sysMode: "vegetative" | "flowering"}
 */
void handleGetData() {
  server.send(200, "application/json", SystemStatus::getInstance().toJson());
}

/**
 * TEMPERATURA
 */
void handleGetTemperature() {
  server.send(200, "text/plain", _sl.toJson("temperature", _sl.data.temperature));
}

/**
 * UMIDADE
 * @todo - juntar temperatura e umidade em uma única função
 */
void handleGetHumidity() {
  server.send(200, "text/plain", _sl.toJson("humidity", _sl.data.humidity));
}

/**
 * TROCA DE MODO 
 * @todo - Implementar modo customizado (setado via client web)
 */
void handleModeChange() {
  if (server.hasArg("plain")) {
    String body = server.arg("plain");  // Obtém o corpo da requisição POST
  
    // Cria um buffer para o JSON
    StaticJsonDocument<200> jsonDocument;
    DeserializationError error = deserializeJson(jsonDocument, body);
  
    // Verifica se o JSON foi parseado corretamente
    if (error) {
      server.send(400, "text/plain", "Erro ao processar JSON: " + String(error.c_str()));
      return;
    }
  
    // Extrai os dados do JSON
    const char* mode = jsonDocument["mode"];
  
    _sl.data.sysMode = mode;
   
    // Prepara a resposta
    String response = "Modo selecionado: " + String(mode) + "\n";
  
    server.send(200, "text/plain", response);
  } else {
    server.send(400, "text/plain", "Corpo da requisição não contém JSON");
  }
}

void setup() {  
    pinMode(D0, OUTPUT);      // LÂMPADA
    pinMode(D4, OUTPUT);      // DHT
    pinMode(D5, OUTPUT);      // VENTILAÇÃO 1 
    pinMode(D7, OUTPUT);      // VENTILAÇÃO 2
  
    Serial.begin(115200);
    
    wifiConfigManager.begin();
    dht.begin();
    
    lcd.init();                       
    lcd.backlight();                  
    lcd.clear();                      

    timeClient.begin();
    timeClient.update();

    if (MDNS.begin("esp8266")) { Serial.println("MDNS responder started"); }

    server.on(F("/data"), HTTP_GET, handleGetData);

    server.on(F("/temperature"), HTTP_GET, handleGetTemperature);

    server.on(F("/humidity"), HTTP_GET, handleGetHumidity);

    server.on(F("/mode"), HTTP_POST, handleModeChange);

    server.begin();
    
    Serial.println("Servidor HTTP iniciado");
}

void updateTime() {
  String horaAtual = timeClient.getFormattedTime();
  String horaMinuto = horaAtual.substring(0, 5);
  _sl.data.dateTime = horaMinuto;
}

void loop() {
  wifiConfigManager.handleClient();
  timeClient.update();
  
  if (wifiConfigManager.getConnectionStatus()) {
      updateTime();
      server.handleClient();
      fan1.update();
      fan2.update();
      lcdDisplay.update();
      
      if(_sl.data.sysMode == "vegetative") {
        lightVeg.update(); 
      } else {
        lightFlo.update();  
      }
  } else {
      lcd.setCursor(0, 0);
      lcd.print("  SEM CONEXAO   ");
      lcd.setCursor(0,1);
      lcd.print("----------------");
  }
}
