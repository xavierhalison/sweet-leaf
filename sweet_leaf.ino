#include <ESP8266WiFi.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>

#include <uri/UriBraces.h>
#include <uri/UriRegex.h>

#include "OutputDevice.h"

ESP8266WebServer server(80);

// Configurações de Wi-Fi
const char* ssid = "Ginasio Fantasma";
const char* password = "Bulbassauro1";

// Configuração do NTPClient
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", -3 * 3600, 60000); // Fuso horário -3 (Brasília)

bool lampToggle = false;

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
    Serial.println(WiFi.localIP());

    // Inicializa o NTPClient
    timeClient.begin();
    timeClient.update();

    if (MDNS.begin("esp8266")) { Serial.println("MDNS responder started"); }

    server.on(F("/"), []() {
      server.send(200, "text/plain", "hello from esp8266!");
      lampToggle = !lampToggle;
      digitalWrite(D0, lampToggle ? HIGH : LOW);
    });

    server.begin();
    Serial.println("HTTP server started");
}

void loop() {
  server.handleClient();
}
