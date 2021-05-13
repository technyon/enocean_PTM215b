/**
 * A BLE client example to receive events from an Enocean PTM215b switch
 * 
 * 
 * author Jeroen
 */
#include "enocean_PTM215b.h"

#define BLE_ADDRESS "E2:15:00:00:D4:AD"
#define SECURITY_KEY "8B49C1B3C62DAE598DCFD4C1AFB1956A"

class EH : public PTM215b::Eventhandler {
public:
    EH() {};
    virtual ~EH() {};

    void handleEvent(PTM215b::SwitchEvent& evt) override {

      std::string type;
      switch (evt.eventType) {
      case PTM215b::EventType::Pushed:
        type = "Pushed";
        break;
      case PTM215b::EventType::Repeat:
        type = "Repeat";
        break;
      case PTM215b::EventType::ReleaseLong:
        type = "ReleasedLong";
        break;
      
      case PTM215b::EventType::ReleaseShort:
        type = "ReleasedShort";
        break;

      default:
        break;
      }

      std::string direction = (evt.direction == PTM215b::Direction::Up) ? "Up" : "Down";

      log_d("BleSwitchEvent Received: Node Id: %d, Type: %s, Direction: %s", evt.nodeId, type.c_str(), direction.c_str());
    };

};

EH handler;
PTM215b::Enocean_PTM215b enocean_PTM215b(true);

void setup(){
  Serial.begin(115200);
  log_d("Starting Enocean_PTM215b BLE Example application...");
    
  BLEDevice::init("ESP32_client");
  enocean_PTM215b.initialize();

  log_d("Adding switch");
  enocean_PTM215b.registerBleSwitch(BLE_ADDRESS, SECURITY_KEY, 10, 11, &handler);
  log_d("Adding switch Done");

}

void loop(){

    delay(1000);

} 