#ifndef SYSTEM_STATUS_H
#define SYSTEM_STATUS_H

#include <ArduinoJson.h>

class SystemStatus {
  private: 
    SystemStatus() {};
  
  public:
    struct SystemData {
      String sysMode = "vegetative";
      float temperature = 0.0;
      float humidity = 0.0;
      String dateTime = "";
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
      json["dateTime"] = data.humidity;

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
