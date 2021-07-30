#include "EnOceanBLEScanner.h"

#define BLE_ADDRESS "E2:15:00:00:D4:AD"
#define SECURITY_KEY "8B49C1B3C62DAE598DCFD4C1AFB1956A"

class Handler : public EnOcean::PTM215EventHandler {
public:
  Handler() {};

  void handleEvent(EnOcean::PTM215Event& event) override {
    log_d("Handling Event");
    log_d("Devicetype: %d", event.device->address);
    log_d("Event: button %d type %d", event.button, event.eventType);
  }
};



EnOcean::BLEScanner scanner;
Handler handler;

void setup(){
  Serial.begin(115200);
  log_d("Starting EnOcean BLE Example application...");

  NimBLEDevice::init("ESP32_client");

  scanner.initialize();
  
  log_d("Adding device");
  scanner.registerPTM215Device(BLE_ADDRESS, SECURITY_KEY, &handler, true, false, true, false);
  log_d("Adding device Done");

}

void loop(){

    delay(1000);

} 