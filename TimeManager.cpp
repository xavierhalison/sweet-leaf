#include "TimeManager.h"

TimeManager::TimeManager() : timeClient(ntpUDP, "pool.ntp.org", -3 * 3600, 60000) {
    timeUpdated = false;
    epochTime = 0;
    lastUpdate = 0;
}

void TimeManager::begin() {
    timeClient.begin();
    if (updateTime()) {
        timeUpdated = true;
    }
}

bool TimeManager::updateTime() {
    if (timeClient.update()) {
        epochTime = timeClient.getEpochTime();
        lastUpdate = millis();
        return true;
    }
    return false;
}

unsigned long TimeManager::getEpochTime() {
    if (timeUpdated) {
        unsigned long currentMillis = millis();
        unsigned long elapsedTime = (currentMillis - lastUpdate) / 1000;
        return epochTime + elapsedTime;
    }
    return 1735689600; // Retorna 01/01/2025 em epoch se o tempo não foi atualizado
}

String TimeManager::getFormattedTime() {
    if (timeUpdated) {
        unsigned long currentEpochTime = getEpochTime();
        time_t rawTime = currentEpochTime;
        struct tm *ptm = gmtime(&rawTime);
        char buffer[40];
        sprintf(buffer, "%02d/%02d/%04d %02d:%02d:%02d", ptm->tm_year + 1900, ptm->tm_mon + 1, ptm->tm_mday, ptm->tm_hour, ptm->tm_min, ptm->tm_sec);
        return String(buffer);
    }
    return "00:00:00"; // Retorna um valor padrão se o tempo não foi atualizado
}

bool TimeManager::isTimeUpdated() {
    return timeUpdated;
}
