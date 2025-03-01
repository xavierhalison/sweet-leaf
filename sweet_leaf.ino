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
    _sl.data.sensorStatus = false;
    return;
    
  } else {
    if (t >= 26.0) { 
      digitalWrite(D5, LOW); 
      _sl.data.fan1Status = true;
      
    } else {
      digitalWrite(D5, HIGH); 
      _sl.data.fan1Status = false;
    }
    
    _sl.data.temperature    = t;
    _sl.data.humidity       = h;
    _sl.data.sensorStatus   = true;
    return;
  }
}

void handleLcd(String firstLine, String secondLine) {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(firstLine);
  lcd.setCursor(0,1);
  lcd.print(secondLine);
}

/**
 * Display LCD
 * Pinagem lcd i2c * Vcc = Vin * gnd = gnd * sda = D3 * scl = D2 * Endereço 0x27 * Ajustar o contraste se não estiver visível
 * @todo implementar modo tela de erro 
 */
void onLcdChange(int currentIndex) {
  String sysMode = "";
  
  if(String(_sl.data.sysMode) == "vegetative") sysMode    = "MODO VEGETATIVO ";
  else if (String(_sl.data.sysMode) == "bloom") sysMode   = " MODO FLORACAO  ";
  else if (_sl.data.sysMode == "debug") sysMode           = "   MODO DEBUG   ";
  else sysMode                                            = "      ERRO      ";

  String firstLine = "";
  String secondLine = "";

  if (_sl.data.sysMode != "debug") {
    switch(currentIndex) {
      case 0: 
        updateTime();
        secondLine = "     " + _sl.data.dateTime + "      ";
        handleLcd(sysMode, secondLine);
        break;
      case 1: 
        firstLine   = "    UMIDADE:    ";
        secondLine  = "     " + String(_sl.data.humidity) + "%     ";
        handleLcd(firstLine, secondLine);
        break;
      case 2: 
        firstLine   = "  TEMPERATURA:   ";
        secondLine  = "     " + String(_sl.data.temperature) + "C     ";
        handleLcd(firstLine, secondLine);
        break;
    }    
  } else {
    switch(currentIndex) {
      case 0: 
        updateTime();
        firstLine   = "Hora       " + _sl.data.dateTime;
        secondLine  = "Modo       " + _sl.data.sysMode;
        handleLcd(firstLine, secondLine);
        break;
      case 1: 
        firstLine   = wifiConfigManager.getConnectionStatus()   ? "Internet      ON" : "Internet     OFF";
        secondLine  = _sl.data.sensorStatus                     ? "Sensor        ON" : "Sensor       OFF";
        handleLcd(firstLine, secondLine);
        break;
      case 2: 
        firstLine   = _sl.data.fan1Status   ? "Fan 1         ON" : "Fan 1        OFF";
        secondLine  = _sl.data.fan2Status   ? "Fan 2         ON" : "Fan 2        OFF";
        handleLcd(firstLine, secondLine);
        break;
      case 3: 
        firstLine   = "Temp       " + String(_sl.data.temperature);
        secondLine  = "Umid       " + String(_sl.data.humidity);
        handleLcd(firstLine, secondLine);
        break;
      case 4: 
        firstLine   = _sl.data.lightStatus   ? "Luz           ON" : "Luz          OFF";
        secondLine  = "   Sweet Leaf   ";
        handleLcd(firstLine, secondLine);
        break;        
    }      
  }
}

/**
 * LÂMPADA PRINCIPAL
 */
void onLightToggle(int currentIndex) {
  Serial.println(currentIndex);
  bool lightOn          = currentIndex % 2 == 0;
  _sl.data.lightStatus   = lightOn;
  
  digitalWrite(D0, lightOn ? HIGH : LOW); 
}

/**
 * PROVISÓRIO: SISTEMA DE CIRCULAÇÃO DO AR
 * enquanto o sistema não é instalado, apenas acender um led
 */
void onFan2Toggle(int currentIndex) {
  bool fanOn            = currentIndex % 2 == 0;
  _sl.data.fan2Status   = fanOn;
  
  digitalWrite(D7, fanOn ? LOW : HIGH); 
}


///// INSTANCIAS DO GERENCIADOR DE DISPOSITIVOS ///// 

/**
 * DISPLAY LCD
 */
unsigned long lcdIntervals[]        = {10, 5, 5};
unsigned long lcdDebugIntervals[]   = {2, 2, 2, 2, 2};
unsigned long fan1Interval[]        = {30}; 
unsigned long lightIntervalVeg[]    = {8 * 60 * 60, 16 * 60 * 60};
unsigned long lightIntervalFlo[]    = {12 * 60 * 60, 12 * 60 * 60};
unsigned long lightIntervaldebug[]  = {18 * 60 * 60, 6 * 60 * 60};
unsigned long fan2Interval[]        = {30, 5 * 60}; 

OutputDevice lcdDisplay(lcdIntervals, 3, false, onLcdChange, &timeClient);
OutputDevice lcdDebugDisplay(lcdDebugIntervals, 5, false, onLcdChange, &timeClient);

/**
 * VENTILAÇÃO 1
 * O ventilador será acionado cada vez que o sensor dht ler uma temperatura >= 26°
 * por meio do callback onSensorCheck
 */
OutputDevice fan1(fan1Interval, 1, false, onSensorCheck, &timeClient);

/**
 * LÂMPADA
 * Cada modo de operação possúi sua instância
 * startAtMidnight = true, para que os horários da lâmpada sejam humanamente previsíveis
 * x * 60 * 60 = quantidade de segundos em x horas
 */
OutputDevice lightVeg(lightIntervalVeg, 2, true, onLightToggle, &timeClient);
OutputDevice lightFlo(lightIntervalFlo, 2, true, onLightToggle, &timeClient);
OutputDevice lightDebug(lightIntervaldebug, 2, true, onLightToggle, &timeClient);

/**
 * VENTILAÇÃO 2 
 * @todo - implementar modo vegetativo e de floração para esse dispositivo
 */
OutputDevice fan2(fan2Interval, 2, false, onFan2Toggle, &timeClient);


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

    String starting = "Iniciando.......";

    lcd.init();                       
    lcd.backlight();                  
    lcd.clear();   
  
    Serial.begin(115200);
    
    handleLcd(starting, "WiFi");
    wifiConfigManager.begin();
    delay(1000);
    
    handleLcd(starting, "Sensor");
    dht.begin();                   
    delay(1000);
  
    handleLcd(starting, "Servidor NTP");
    timeClient.begin();
    timeClient.update();
    delay(1000);

    handleLcd(starting, "Servidor HTTP");
    if (MDNS.begin("esp8266")) { Serial.println("MDNS responder started"); }
    delay(1000);

    server.on(F("/data"), HTTP_GET, handleGetData);

    server.on(F("/temperature"), HTTP_GET, handleGetTemperature);

    server.on(F("/humidity"), HTTP_GET, handleGetHumidity);

    server.on(F("/mode"), HTTP_POST, handleModeChange);

    server.begin();
    
    Serial.println("Servidor HTTP iniciado");
}

void updateTime() {
  String h = timeClient.getFormattedTime();
  String hStr = h.substring(0, 5);
  _sl.data.dateTime = hStr;
}

void loop() {
  wifiConfigManager.handleClient();

  if(_sl.data.sysMode == "debug") {
    lcdDebugDisplay.update();
  } else {
    lcdDisplay.update(); 
  }

  if(wifiConfigManager.getConnectionStatus()) {
    server.handleClient();
    timeClient.update();
    updateTime();
    
    fan1.update();
    fan2.update();

    if(_sl.data.sysMode == "vegetative") {
      lightVeg.update(); 
    } else if (_sl.data.sysMode == "flowering"){
      lightFlo.update();  
    } else {
      lightDebug.update();
    }
  }
}
