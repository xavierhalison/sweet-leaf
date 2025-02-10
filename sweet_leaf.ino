#include <ESP8266WiFi.h>
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
#include "OutputDevice.h"

#define DHTPIN D4
#define DHTTYPE    DHT22     // DHT 22 (AM2302)

const char* ssid = "Ginasio Fantasma";
const char* password = "Bulbassauro1";

DHT dht(DHTPIN, DHTTYPE);
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", -3 * 3600, 60000); // Fuso horário -3 (Brasília)
ESP8266WebServer server(80);

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
    } else {
      _sl.data.temperature = t;
      _sl.data.humidity = h;
    }
}

unsigned long sensorInterval[] = {30}; // tempo de atualização do sensor: 30 segundos
OutputDevice sensor(sensorInterval, 1, false, onSensorCheck, &timeClient);

bool lampToggle = false;
void onLightToggle(int currentIndex) {
    lampToggle = !lampToggle;
    digitalWrite(D0, lampToggle ? HIGH : LOW);
}

unsigned long lightIntervalVeg[] = {10};
unsigned long lightIntervalFlo[] = {30};

OutputDevice lightVeg(lightIntervalVeg, 1, false, onLightToggle, &timeClient);
OutputDevice lightFlo(lightIntervalFlo, 1, false, onLightToggle, &timeClient);

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
  
    Serial.begin(115200);

    dht.begin();

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

    server.on(F("/"), HTTP_GET, handleGetData);

    server.on(F("/temperature"), HTTP_GET, handleGetTemperature);

    server.on(F("/humidity"), HTTP_GET, handleGetHumidity);

    server.on(F("/mode"), HTTP_POST, handleModeChange);

    server.begin();
    Serial.println("HTTP server started");
}

void loop() {
  server.handleClient();
  sensor.update();
  if(_sl.data.sysMode == "vegetative") {
    lightVeg.update(); 
  } else {
    lightFlo.update();  
  }
  delay(100);
}
