#include <ESP8266WiFi.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <DHT_U.h>
#include "OutputDevice.h"

#define DHTPIN D4
#define DHTTYPE    DHT22     // DHT 22 (AM2302)
DHT dht(DHTPIN, DHTTYPE);

// Configurações de Wi-Fi
const char* ssid = "Ginasio Fantasma";
const char* password = "Bulbassauro1";

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
    } else {
      Serial.println(h);
      Serial.println(t);
      Serial.println();
      return;
    }
}

// Array de intervalos de tempo (em segundos)
unsigned long sensorInterval[] = {30}; // tempo de atualização do sensor: 30 segundos

OutputDevice sensor(sensorInterval, 1, false, onSensorCheck, &timeClient);

void setup() {
    pinMode(D1, OUTPUT);
  
    Serial.begin(115200);

    dht.begin();

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
}
