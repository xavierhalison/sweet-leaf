#ifndef OUTPUTDEVICE_H
#define OUTPUTDEVICE_H

#include <Arduino.h>
#include <WiFiUdp.h>
#include "TimeManager.h"

class OutputDevice {
private:
    unsigned long* intervals; // Array de intervalos de tempo (em segundos)
    int intervalCount; // Número de intervalos
    int currentIntervalIndex; // Índice do intervalo atual
    unsigned long lastToggleTime; // Último momento em que o dispositivo foi alterado (em segundos)
    unsigned long nextToggleTime;
    bool startAtMidnight;
    void (*onToggleCallback)(int); // Callback para quando o estado muda
    TimeManager* timeClient; // Ponteiro para o objeto NTPClient;

    unsigned long getMidnight();
    void increaseIntervalIndex();
    void handleMidnightCicles(unsigned long currentTime);
    
public:
    // Construtor
    OutputDevice(unsigned long* inter, int count, bool midnight, void (*callback)(int), TimeManager* client);

    // Atualiza o estado do dispositivo
    void update();
};

#endif
