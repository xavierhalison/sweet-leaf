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

void onSensorCheck(int currentIndex) {
  Serial.println("temperature sensor check");
}

void onLightToggle(int currentIndex) {
  bool ledOn = currentIndex % 2 == 0;
  digitalWrite(D1, ledOn ? HIGH : LOW); 
}

// Array de intervalos de tempo (em segundos)
unsigned long sensorInterval[] = {30}; // tempo de atualização do sensor: 30 segundos
unsigned long lightIntervals[] = {8*60*60, 16*60*60}; // intervalos de acionamento da lâmpada (vegetativo): 8/16 horas;

//unsigned long lightIntervals[] = {12*60*60, 12*60*60}; // intervalos de acionamento da lâmpada (floração) 12/12 horas;

// Cria um dispositivo no pino D1, com os intervalos e o callback
OutputDevice sensor(sensorInterval, 1, false, onSensorCheck, &timeClient);
OutputDevice lights(lightIntervals, 2, true, onLightToggle, &timeClient);

void setup() {
    pinMode(D0, OUTPUT);
    pinMode(D1, OUTPUT);
  
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
