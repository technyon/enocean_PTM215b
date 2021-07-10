/**
 * A BLE client example to receive events from an Enocean PTM215b switch
 * 
 * 
 * author Jeroen
 */
#include "enocean_PTM215b.h"

#define BLE_ADDRESS "E2:15:00:00:D4:AD"
#define SECURITY_KEY "8B49C1B3C62DAE598DCFD4C1AFB1956A"

class PtmEventhandler : public PTM215b::Eventhandler {
public:
    PtmEventhandler() {};
    virtual ~PtmEventhandler() {};

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


PtmEventhandler handler;
PTM215b::Enocean_PTM215b enocean_PTM215b(true);

class PtmCommissioninghandler : public PTM215b::CommissioningEventhandler {
public:
    PtmCommissioninghandler() {};
    virtual ~PtmCommissioninghandler() {};

    void handleEvent(PTM215b::CommissioningEvent& evt) override {
      log_d("Commissioning event received");
      enocean_PTM215b.registerBleSwitch(evt.address, evt.securityKey, 20, 21, &handler);
    };

};

PtmCommissioninghandler commHandler;

void setup(){
  Serial.begin(115200);
  log_d("Starting Enocean_PTM215b BLE Example application...");
    
  NimBLEDevice::init("ESP32_client");
  enocean_PTM215b.initialize();
  enocean_PTM215b.setCommissioningEventHandler(&commHandler);

  // log_d("Adding switch");
  // enocean_PTM215b.registerBleSwitch(BLE_ADDRESS, SECURITY_KEY, 10, 11, &handler);
  // log_d("Adding switch Done");

}

void loop(){

    delay(1000);

} 