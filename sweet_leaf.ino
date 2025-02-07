#include <ESP8266WiFi.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include "OutputDevice.h"

// Configurações de Wi-Fi
const char* ssid = "Ginasio Fantasma";
const char* password = "Bulbassauro1";

// Configuração do NTPClient
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", -3 * 3600, 60000); // Fuso horário -3 (Brasília)

bool toggleSensor = true;
void onSensorCheck(int currentIndex) {
  digitalWrite(D0, toggleSensor ? HIGH: LOW);
  toggleSensor = !toggleSensor;
}

void onLightToggle(int currentIndex) {
  bool ledOn = currentIndex % 2 == 0;
  digitalWrite(D1, ledOn ? HIGH : LOW); 
}

// Array de intervalos de tempo (em segundos)
unsigned long sensorInterval[] = {60};
unsigned long lightIntervals[] = {5, 1, 3};

// Cria um dispositivo no pino D1, com os intervalos e o callback
OutputDevice sensor(sensorInterval, 1, false, onSensorCheck, &timeClient);
OutputDevice lights(lightIntervals, 2, true, onLightToggle, &timeClient);

void setup() {
    pinMode(D0, OUTPUT);
    pinMode(D1, OUTPUT);
//
//    digitalWrite(D0, HIGH);
//    digitalWrite(D1, HIGH);
  
    Serial.begin(115200);

    // Conecta ao Wi-Fi
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(1000);
        Serial.println("Conectando ao Wi-Fi...");
    }
    Serial.println("Conectado ao Wi-Fi!");

    // Inicializa o NTPClient
    timeClient.begin();
    timeClient.update();
}

void loop() {
    sensor.update();
    lights.update();
}
