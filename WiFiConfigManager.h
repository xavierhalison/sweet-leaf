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

  // Retorna o estado da conexão Wi-Fi
  bool getConnectionStatus(); // Retorna true se estiver conectado, false caso contrário

private:
  const char* _apSSID;      // Nome do Access Point
  const char* _apPassword;  // Senha do Access Point
  ESP8266WebServer& _server; // Referência ao Web Server (passado externamente)

  // Métodos privados
  void handleRoot();        // Página inicial (formulário de configuração)
  void handleSave();        // Salva as credenciais e tenta conectar ao Wi-Fi
  void startAP();           // Inicia o Access Point
  bool connectToWiFi(const char* ssid, const char* password); // Conecta ao Wi-Fi

  // Novas funções para gerenciar credenciais na EEPROM
  void saveCredentials(const String& ssid, const String& password); // Salva credenciais na EEPROM
  bool loadCredentials(String& ssid, String& password); // Carrega credenciais da EEPROM
  bool autoConnect(); // Tenta conectar automaticamente ao Wi-Fi com as credenciais salvas
};

// Implementação dos métodos
WiFiConfigManager::WiFiConfigManager(const char* apSSID, const char* apPassword, ESP8266WebServer& server)
  : _apSSID(apSSID), _apPassword(apPassword), _server(server) {
  EEPROM.begin(512); // Inicializa a EEPROM com 512 bytes
}

void WiFiConfigManager::begin() {
  // Tenta conectar automaticamente ao Wi-Fi com as credenciais salvas
  if (!autoConnect()) {
    startAP(); // Inicia o Access Point se não conseguir conectar
  }

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
    saveCredentials(ssid, password); // Salva as credenciais na EEPROM
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

// Implementação da nova função
bool WiFiConfigManager::getConnectionStatus() {
  return WiFi.status() == WL_CONNECTED; // Retorna true se estiver conectado, false caso contrário
}

// Função para salvar as credenciais na EEPROM
void WiFiConfigManager::saveCredentials(const String& ssid, const String& password) {
  Serial.println("Salvando credenciais na EEPROM...");

  // Limpa a EEPROM antes de salvar
  for (int i = 0; i < 96; ++i) {
    EEPROM.write(i, 0);
  }

  // Salva o SSID
  for (int i = 0; i < ssid.length(); ++i) {
    EEPROM.write(i, ssid[i]);
  }

  // Salva a senha
  for (int i = 0; i < password.length(); ++i) {
    EEPROM.write(32 + i, password[i]);
  }

  EEPROM.commit(); // Confirma a escrita na EEPROM
  Serial.println("Credenciais salvas!");
}

// Função para carregar as credenciais da EEPROM
bool WiFiConfigManager::loadCredentials(String& ssid, String& password) {
  Serial.println("Carregando credenciais da EEPROM...");

  ssid = "";
  password = "";

  // Lê o SSID
  for (int i = 0; i < 32; ++i) {
    char c = EEPROM.read(i);
    if (c == 0) break;
    ssid += c;
  }

  // Lê a senha
  for (int i = 32; i < 96; ++i) {
    char c = EEPROM.read(i);
    if (c == 0) break;
    password += c;
  }

  if (ssid.length() > 0 && password.length() > 0) {
    Serial.println("Credenciais carregadas:");
    Serial.println("SSID: " + ssid);
    Serial.println("Senha: " + password);
    return true;
  } else {
    Serial.println("Nenhuma credencial encontrada na EEPROM.");
    return false;
  }
}

// Função para tentar conectar automaticamente ao Wi-Fi
bool WiFiConfigManager::autoConnect() {
  String ssid, password;
  if (loadCredentials(ssid, password)) {
    return connectToWiFi(ssid.c_str(), password.c_str());
  } else {
    Serial.println("Nenhuma credencial salva. Inicie o modo AP para configurar.");
    return false;
  }
}

#endif
