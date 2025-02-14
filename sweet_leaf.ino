#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <DHT_U.h>
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
      digitalWrite(D1, HIGH);
      return;
    } else if (t >= 26.0) { 
      digitalWrite(D5, HIGH);
      return;
    }
}


void onLightToggle(int currentIndex) {
  bool ledOn = currentIndex % 2 == 0;
  digitalWrite(D1, ledOn ? HIGH : LOW); 
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


void setup() {
    pinMode(D1, OUTPUT);
    pinMode(D4, OUTPUT);
    pinMode(D5, OUTPUT);
    pinMode(D6, OUTPUT);
  
    Serial.begin(115200);
    dht.begin();

    wifiConfigManager.begin();
    

    // Inicializa o NTPClient
    timeClient.begin();
    timeClient.update();
}

void loop() {
    wifiConfigManager.handleClient();

   lights.update();
   fan1.update();
   fan2.update();
}
