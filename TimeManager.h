#ifndef TIMEMANAGER_H
#define TIMEMANAGER_H

#include <NTPClient.h>
#include <WiFiUdp.h>

class TimeManager {
private:
    WiFiUDP ntpUDP;
    NTPClient timeClient;
    unsigned long lastUpdate;
    unsigned long epochTime;
    bool timeUpdated;

public:
    TimeManager();
    void begin();
    bool updateTime();
    unsigned long getEpochTime();
    String getFormattedTime();
    bool isTimeUpdated();
};

#endif
