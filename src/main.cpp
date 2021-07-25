#include "EnoceanBLEScanner.h"

#define BLE_ADDRESS "E2:15:00:00:D4:AD"
#define SECURITY_KEY "8B49C1B3C62DAE598DCFD4C1AFB1956A"

class Handler : public Enocean::PayloadHandler {
public:
  Handler() {};

  void handleEvent(Enocean::DeviceType type, void* event) override {
    log_d("Handling Event");
    log_d("Devicetype: %d", type);
    Enocean::PTM215Event evt = *(Enocean::PTM215Event*)event;
   log_d("Event: dir %d type %d", evt.direction, evt.eventType);
  }
};



Enocean::BLEScanner scanner;
Handler handler;

void setup(){
  Serial.begin(115200);
  log_d("Starting Enocean BLE Example application...");

  NimBLEDevice::init("ESP32_client");

  scanner.initialize();
  
  log_d("Adding device");
  scanner.registerDevice(BLE_ADDRESS, SECURITY_KEY, &handler);
  log_d("Adding device Done");

}

void loop(){

    delay(1000);

} 