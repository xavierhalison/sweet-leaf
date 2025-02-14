#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <DHT_U.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include "OutputDevice.h"
#include "WiFiConfigManager.h"

// Defina o nome e a senha do Access Point
const char* apSSID = "Sweet Leaf";
const char* apPassword = "42002420";

ESP8266WebServer server(80);

WiFiConfigManager wifiConfigManager(apSSID, apPassword, server);

#define DHTPIN D4
#define DHTTYPE    DHT22     // DHT 22 (AM2302)
DHT dht(DHTPIN, DHTTYPE);


// Configuração do NTPClient
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", -3 * 3600, 60000); // Fuso horário -3 (Brasília)

LiquidCrystal_I2C lcd(0x27, 16, 2);

void onSensorCheck(int currentIndex) {
    // Reading temperature or humidity takes about 250 milliseconds!
    // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
    float h = dht.readHumidity();
    // Read temperature as Celsius (the default)
    float t = dht.readTemperature();
    // Read temperature as Fahrenheit (isFahrenheit = true)
  
    // Check if any reads failed and exit early (to try again).
    if (isnan(h) || isnan(t)) {
      Serial.println(F("Failed to read from DHT sensor!"));
      return;
    } else if (t >= 26.0) { 
      digitalWrite(D5, HIGH);
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
      lcd.print("  EH O WHISKAS  ");
      lcd.setCursor(0,1);
      lcd.print("    CARALHO!    ");
      break;
    case 1: 
      lcd.setCursor(0, 0);
      lcd.setCursor(0,1);
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

unsigned long lightIntervals[] = {10, 1}; // intervalos de acionamento da lâmpada (vegetativo): 8/16 horas;
OutputDevice lights(lightIntervals, 2, false , onLightToggle, &timeClient);

unsigned long fan1Interval[] = {30}; // tempo de atualização do sensor: 30 segundos
OutputDevice fan1(fan1Interval, 1, false, onSensorCheck, &timeClient);

unsigned long fan2Interval[] = {10,1}; // tempo de atualização do sensor: 30 segundos
OutputDevice fan2(fan2Interval, 2, false, onFan2Toggle, &timeClient);

unsigned long lcdIntervals[] = {3, 1};
OutputDevice lcdDisplay(lcdIntervals, 2, false, onLcdChange, &timeClient);


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
}

void loop() {
    wifiConfigManager.handleClient();

   lights.update();
   fan1.update();
   fan2.update();
   lcdDisplay.update();
}
