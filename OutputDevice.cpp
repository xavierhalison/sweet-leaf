#include "OutputDevice.h"

// Construtor
OutputDevice::OutputDevice(unsigned long* inter, int count, bool midnight, void (*callback)(int), NTPClient* client) {
    intervals = inter;
    intervalCount = count;
    currentIntervalIndex = 0;
    onToggleCallback = callback;
    timeClient = client;
    nextToggleTime = 0;
    startAtMidnight = midnight;
}

unsigned long OutputDevice::getMidnight() {
    // Obter o tempo atual em segundos
    unsigned long epochTime = timeClient->getEpochTime();

    // Calcular o horário de 00:00 do dia atual
    unsigned long midnightToday = (epochTime / 86400L) * 86400L;

    return midnightToday + 86400L;

    // return midnightToday + (20 * 3600) + (42 * 60); // 20:40 today
}
// Atualiza o estado do dispositivo
void OutputDevice::update() {
    timeClient->update(); // Atualiza o tempo do NTPClient
    unsigned long currentTime = timeClient->getEpochTime(); // Obtém o tempo atual em segundos

    if(startAtMidnight && nextToggleTime == 0) {
      nextToggleTime = getMidnight();
    } 
    
    if (currentTime >= nextToggleTime) {
      if(onToggleCallback) {
        onToggleCallback(currentIntervalIndex);
      }
      lastToggleTime = currentTime;
      nextToggleTime = lastToggleTime + intervals[currentIntervalIndex];
      currentIntervalIndex++;
      if (currentIntervalIndex >= intervalCount) {
        currentIntervalIndex = 0; // Volta ao início do array de intervalos
      }
    }
}
