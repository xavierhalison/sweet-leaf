#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include "OutputDevice.h"
#include "WiFiConfigManager.h"

// Defina o nome e a senha do Access Point
const char* apSSID = "Sweet Leaf";
const char* apPassword = "42002420";

ESP8266WebServer server(80);

WiFiConfigManager wifiConfigManager(apSSID, apPassword, server);

// Configuração do NTPClient
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", -3 * 3600, 60000); // Fuso horário -3 (Brasília)

void onSensorCheck(int currentIndex) {
  Serial.println("temperature sensor check");
}

void onLightToggle(int currentIndex) {
  bool ledOn = currentIndex % 2 == 0;
  digitalWrite(D1, ledOn ? HIGH : LOW); 
}

// Array de intervalos de tempo (em segundos)
unsigned long lightIntervals[] = {10, 1}; // intervalos de acionamento da lâmpada (vegetativo): 8/16 horas;

OutputDevice lights(lightIntervals, 2, false , onLightToggle, &timeClient);

void setup() {
    pinMode(D1, OUTPUT);
  
    Serial.begin(115200);

    wifiConfigManager.begin();

    // Inicializa o NTPClient
    timeClient.begin();
    timeClient.update();
}

void loop() {
    wifiConfigManager.handleClient();

    delay(3000);
  
   //sensor.update();
   lights.update();
}
