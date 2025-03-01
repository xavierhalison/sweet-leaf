#ifndef SYSTEM_STATUS_H
#define SYSTEM_STATUS_H

#include <ArduinoJson.h>

class SystemStatus {
  private: 
    SystemStatus() {};
  
  public:
    struct SystemData {
      String sysMode = "debug";
      float temperature = 0.0;
      float humidity = 0.0;
      String dateTime = "";
      bool sensorStatus = false;
      bool fan1Status = false;
      bool fan2Status = false;
      bool lightStatus = false;
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
      json["dateTime"] = data.dateTime;
      json["sensorStatus"] = data.sensorStatus;
      json["fan1Status"] = data.fan1Status;
      json["fan2Status"] = data.fan2Status;
      json["lightStatus"] = data.fan2Status;

      String jsonString;
      serializeJson(json, jsonString);
      return jsonString;
    }

    template <typename T>
    String toJson(const String& key, T value) {
      StaticJsonDocument<64> json;
      json[key] = value;

      String jsonString;
      serializeJson(json, jsonString);
      return jsonString;
    }
};

#endif // SYSTEM_STATUS_H
