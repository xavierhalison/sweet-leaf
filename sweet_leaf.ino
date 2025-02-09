#include <ESP8266WiFi.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include "OutputDevice.h"

// Configurações de Wi-Fi
const char* ssid = "Ginasio Fantasma";
const char* password = "Bulbassauro1";

// Configuração do NTPClient
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", -3 * 3600, 60000); // Fuso horário -3 (Brasília)


/**
 * Pinagem lcd i2c
 * Vcc = Vin
 * gnd = gnd
 * sda = D3
 * scl = D2
 * Endereço 0x27
 * Ajustar o contraste se não estiver visível
 */
LiquidCrystal_I2C lcd(0x27, 16, 2);

void onLcdChange(int currentIndex) {
    lcd.clear();
  switch(currentIndex) {
    case 0: 
      lcd.setCursor(0, 0);
      lcd.print("  TEMPERATURA:  ");
      lcd.setCursor(0,1);
      lcd.print("      xº C      ");
      break;
    case 1: 
      lcd.setCursor(0, 0);
      lcd.print("    UMIDADE:    ");
      lcd.setCursor(0,1);
      lcd.print("       x%       ");
      break;
    case 2: 
      lcd.setCursor(0, 0);
      lcd.print("   SWEET LEAF   ");
      lcd.setCursor(0,1);
      lcd.print("     08:53      ");
      break;
  }  
}

// Array de intervalos de tempo (em segundos)
unsigned long lcdIntervals[] = {10, 10, 10};

OutputDevice lcdDisplay(lcdIntervals, 3, false, onLcdChange, &timeClient);

void setup() {  
    Serial.begin(115200);

    lcd.init();                       // Initialize the LCD
    lcd.backlight();                  // Turn on the backlight
    lcd.clear();                      // Clear the LCD screen

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
    lcdDisplay.update();
}
