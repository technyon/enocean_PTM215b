/**
 * A BLE client example to receive events from an Enocean PTM215b switch
 * 
 * 
 * author Jeroen
 */

#include "enocean_PTM215b.h"

#define STRINGIFY_(x) #x
#define STRING(VAR) STRINGIFY_(VAR)
#define BLE_ADDRESS "e2:15:00:01:72:f9"
#define SECURITY_KEY "1CB95CE63F19ADC7E0FB92DA56D69219"

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

void setup(){
  Serial.begin(115200);
  log_d("Starting Enocean_PTM215b BLE Example application...");
    
  BLEDevice::init("ESP32_client");
  enocean_PTM215b.initialize();

  enocean_PTM215b.registerBleSwitch(STRING(BLE_ADDRESS), STRING(SECURITY_KEY), 0, 1);

}

void loop(){

    delay(1000);

} 