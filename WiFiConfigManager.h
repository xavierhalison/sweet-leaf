#ifndef WIFICONFIGMANAGER_H
#define WIFICONFIGMANAGER_H

#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <EEPROM.h>

class WiFiConfigManager {
public:
  // Construtor: recebe o servidor como parâmetro
  WiFiConfigManager(const char* apSSID, const char* apPassword, ESP8266WebServer& server);

  // Inicializa o Access Point e o Web Server
  void begin();

  // Processa requisições do cliente
  void handleClient();

private:
  const char* _apSSID;      // Nome do Access Point
  const char* _apPassword;  // Senha do Access Point
  ESP8266WebServer& _server; // Referência ao Web Server (passado externamente)

  // Métodos privados
  void handleRoot();        // Página inicial (formulário de configuração)
  void handleSave();        // Salva as credenciais e tenta conectar ao Wi-Fi
  void startAP();           // Inicia o Access Point
  bool connectToWiFi(const char* ssid, const char* password); // Conecta ao Wi-Fi
};

// Implementação dos métodos
WiFiConfigManager::WiFiConfigManager(const char* apSSID, const char* apPassword, ESP8266WebServer& server)
  : _apSSID(apSSID), _apPassword(apPassword), _server(server) {}

void WiFiConfigManager::begin() {
  startAP(); // Inicia o Access Point

  // Configura as rotas do Web Server
  _server.on("/", std::bind(&WiFiConfigManager::handleRoot, this));
  _server.on("/save", std::bind(&WiFiConfigManager::handleSave, this));

  _server.begin(); // Inicia o Web Server
  Serial.println("Web Server iniciado!");
}

void WiFiConfigManager::handleClient() {
  _server.handleClient(); // Processa requisições do cliente
}

void WiFiConfigManager::startAP() {
  WiFi.softAP(_apSSID, _apPassword);
  IPAddress ip = WiFi.softAPIP();
  Serial.print("IP do AP: ");
  Serial.println(ip);
}

void WiFiConfigManager::handleRoot() {
  String html = "<form method='post' action='/save'>"
                "SSID: <input type='text' name='ssid'><br>"
                "Senha: <input type='password' name='password'><br>"
                "<input type='submit' value='Conectar'>"
                "</form>";
  _server.send(200, "text/html", html);
}

void WiFiConfigManager::handleSave() {
  String ssid = _server.arg("ssid");
  String password = _server.arg("password");

  if (connectToWiFi(ssid.c_str(), password.c_str())) {
    _server.send(200, "text/plain", "Conectado à rede Wi-Fi!");
  } else {
    _server.send(200, "text/plain", "Falha ao conectar à rede Wi-Fi.");
  }
}

bool WiFiConfigManager::connectToWiFi(const char* ssid, const char* password) {
  WiFi.begin(ssid, password);
  Serial.print("Conectando à rede Wi-Fi...");

  int tentativas = 0;
  while (WiFi.status() != WL_CONNECTED && tentativas < 20) { // Timeout de 10 segundos
    delay(500);
    Serial.print(".");
    tentativas++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nConectado à rede Wi-Fi!");
    Serial.print("IP: ");
    Serial.println(WiFi.localIP());
    return true;
  } else {
    Serial.println("\nFalha ao conectar à rede Wi-Fi.");
    return false;
  }
}

#endif
