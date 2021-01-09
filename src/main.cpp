/**
 * A BLE client example to receive events from an Enocean PTM215b switch
 * 
 * NOTE: Don't forget to update the properties of your switch in data/bleswitchesconfig.json and flash the ESP32 filesystem with the data folder
 * 
 * author Jeroen
 */

#include "enocean_PTM215b.h"
#include "ArduinoJson.h"
#include "SPIFFS.h"

class EH : public PTM215b::Eventhandler {
public:
    EH() {};
    virtual ~EH() {};

    void handleEvent(PTM215b::BleSwitchEvent& evt) override {

      std::string type;
      switch (evt.eventType) {
      case PTM215b::EventType::Pushed:
        type = "Pushed";
        break;
      case PTM215b::EventType::Repeat:
        type = "Repeat";
        break;
      case PTM215b::EventType::Released:
        type = "Released";
        break;
      
      default:
        break;
      }

      std::string direction = (evt.direction == PTM215b::Direction::Up) ? "Up" : "Down";

      log_d("BleSwitchEvent Received: Node Id: %d, Type: %s, Direction: %s, PushStartTime: %d", evt.nodeId, type.c_str(), direction.c_str(), evt.pushStartTime);
    };

};

EH handler;
PTM215b::Enocean_PTM215b enocean_PTM215b(handler);

DynamicJsonDocument readConfigFile(File& file) {
  file.seek(0);
  DynamicJsonDocument doc(8192);
  DeserializationError error = deserializeJson(doc, file);
  if (error) {
    log_e("Couldn't parse JSON file [%s]: %s", file.name(), error.c_str());
  }
  return doc;
}

void readSettingsFromJSON(File& file) {
  DynamicJsonDocument jsonDocument = readConfigFile(file);

  if (jsonDocument.isNull()) {
    log_e("ERROR reading config file");
    return;
  }
  
  uint8_t counter = 0;
  JsonArray bleSwitchesConfig = jsonDocument["bleSwitchesConfig"];
  for (JsonVariantConst bleSwitchConfig : bleSwitchesConfig) {
    counter++;
    std::string bleAddress = bleSwitchConfig["bleAddress"];
    if (bleAddress == "") {
      log_w("No bleAddress specified");
      continue;
    }
    std::string securityKey = bleSwitchConfig["securityKey"];
    if ((securityKey == "") || (securityKey.length() != 32)) {
      log_w("Invalid securityKey specified");
      continue;
    }
    uint8_t nodeIdA = bleSwitchConfig["nodeIdA"];
    if (!nodeIdA) {
      log_w("No nodeId specified");
      continue;
    }
    uint8_t nodeIdB = bleSwitchConfig["nodeIdB"];

    enocean_PTM215b.registerBleSwitch(bleAddress, securityKey, nodeIdA, nodeIdB);
  }
  log_d("Added %d switch(es) from config", counter);
}


void setup(){
  Serial.begin(115200);
  log_d("Starting Enocean_PTM215b BLE Client application...");
    
  BLEDevice::init("ESP32_client");
  enocean_PTM215b.initialize();

  SPIFFS.begin();
  File file = SPIFFS.open("/bleswitchesconfig.json");
  if (!file) {
    log_e("Cannot open file [%s]", file.name());
    return;
  } 
  readSettingsFromJSON(file);

}

void loop(){

    delay(1000);

} 