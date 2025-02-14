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

#define DHTPIN D4
#define DHTTYPE    DHT22     // DHT 22 (AM2302)

// Defina o nome e a senha do Access Point
const char* apSSID = "Sweet Leaf";
const char* apPassword = "42002420";

ESP8266WebServer server(80);
WiFiConfigManager wifiConfigManager(apSSID, apPassword, server);

DHT dht(DHTPIN, DHTTYPE);
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", -3 * 3600, 60000); // Fuso horário -3 (Brasília)
LiquidCrystal_I2C lcd(0x27, 16, 2);

class SystemStatus {
  private: 
    SystemStatus() {};
  public:
    struct SystemData {
      String sysMode = "vegetative";
      float temperature = 0.0;
      float humidity = 0.0;  
    } data;

    static SystemStatus& getInstance() {
      static SystemStatus instance;
      return instance;
    }

    String toJson() {
      StaticJsonDocument<256> json;
      json["sysMode"] = data.sysMode;
      json["temperature"] = data.temperature;
      json["humidity"] = data.humidity;

      String jsonString;
      serializeJson(json, jsonString);
      return jsonString;
    }

    template <typename T>
    String toJson(const String& key, T value) {
      StaticJsonDocument<64>  json;
      json[key] = value;

      String jsonString;
      serializeJson(json, jsonString);
      return jsonString;
    }
};

SystemStatus& _sl = SystemStatus::getInstance();

// Reading temperature or humidity takes about 250 milliseconds!
// Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
void onSensorCheck(int currentIndex) {
    // SystemStatus& _sl = SystemStatus::getInstance();
    float h = dht.readHumidity();
    float t = dht.readTemperature();
  
    // Check if any reads failed and exit early (to try again).
    if (isnan(h) || isnan(t)) {
      Serial.println(F("Failed to read from DHT sensor!"));
      return;
    }  else {
      if (t >= 26.0) { 
        digitalWrite(D5, HIGH);
      }
      _sl.data.temperature = t;
      _sl.data.humidity = h;
      return;
    }
}

/**
 * Pinagem lcd i2c
 * Vcc = Vin
 * gnd = gnd
 * sda = D3
 * scl = D2
 * Endereço 0x27
 * Ajustar o contraste se não estiver visível
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

void onLightToggle(int currentIndex) {
  bool ledOn = currentIndex % 2 == 0;
  digitalWrite(D0, ledOn ? HIGH : LOW); 
}

void onFan2Toggle(int currentIndex) {
  bool ledOn = currentIndex % 2 == 0;
  digitalWrite(D6, ledOn ? HIGH : LOW); 
}

unsigned long lightIntervalVeg[] = {10, 1};
unsigned long lightIntervalFlo[] = {30, 1};

OutputDevice lightVeg(lightIntervalVeg, 2, false, onLightToggle, &timeClient);
OutputDevice lightFlo(lightIntervalFlo, 2, false, onLightToggle, &timeClient);

unsigned long fan1Interval[] = {30}; // tempo de atualização do sensor: 30 segundos
OutputDevice fan1(fan1Interval, 1, false, onSensorCheck, &timeClient);

unsigned long fan2Interval[] = {10,1}; // tempo de atualização do sensor: 30 segundos
OutputDevice fan2(fan2Interval, 2, false, onFan2Toggle, &timeClient);

unsigned long lcdIntervals[] = {3, 3, 3};
OutputDevice lcdDisplay(lcdIntervals, 3, false, onLcdChange, &timeClient);

void handleGetData() {
  server.send(200, "application/json", SystemStatus::getInstance().toJson());
}

void handleGetTemperature() {
  server.send(200, "text/plain", _sl.toJson("temperature", _sl.data.temperature));
}

void handleGetHumidity() {
  server.send(200, "text/plain", _sl.toJson("humidity", _sl.data.humidity));
}

void handleModeChange() {
  if (server.method() != HTTP_POST) {
    server.send(405, "text/plain", "Método não permitido");
  } else {
    // Verifica se o conteúdo é JSON
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
      String response = "New mode: " + String(mode) + "\n";

      server.send(200, "text/plain", response);
    } else {
      server.send(400, "text/plain", "Corpo da requisição não contém JSON");
    }
  }
}

void setup() {  
    pinMode(D0, OUTPUT);
    pinMode(D4, OUTPUT);
    pinMode(D5, OUTPUT);
    pinMode(D6, OUTPUT);
  
    Serial.begin(115200);
    dht.begin();
    
    wifiConfigManager.begin();
    
    lcd.init();                       // Initialize the LCD
    lcd.backlight();                  // Turn on the backlight
    lcd.clear();                      // Clear the LCD screen


    // Inicializa o NTPClient
    timeClient.begin();
    timeClient.update();

    if (MDNS.begin("esp8266")) { Serial.println("MDNS responder started"); }

    server.on(F("/"), HTTP_GET, handleGetData);

    server.on(F("/temperature"), HTTP_GET, handleGetTemperature);

    server.on(F("/humidity"), HTTP_GET, handleGetHumidity);

    server.on(F("/mode"), HTTP_POST, handleModeChange);

    server.begin();
    Serial.println("HTTP server started");
}

void loop() {
    wifiConfigManager.handleClient();
    server.handleClient();

   fan1.update();
   fan2.update();
   lcdDisplay.update();
  
  if(_sl.data.sysMode == "vegetative") {
    lightVeg.update(); 
  } else {
    lightFlo.update();  
  }
  delay(100);
}
