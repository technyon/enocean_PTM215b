#include "EnOceanBLEScanner.h"

#define BLE_ADDRESS "E2:15:00:00:D4:AD"
#define SECURITY_KEY "8B49C1B3C62DAE598DCFD4C1AFB1956A"

class Handler : public EnOcean::PTM215EventHandler {
public:
  Handler(int id) : id(id) {};

  void handleEvent(EnOcean::PTM215Event& event) override {
    log_d("Handling Event from %d", id);
    log_d("Devicetype: %d", event.device->address);
    log_d("Event: button %d type %d", event.button, event.eventType);
  }

  int id;
};



EnOcean::BLEScanner scanner;
Handler handler(1);
Handler handler2(2);

void setup(){
  Serial.begin(115200);
  log_d("Starting EnOcean BLE Example application...");

  NimBLEDevice::init("ESP32_client");

  scanner.initialize();
  
  log_d("Adding device");
  scanner.registerPTM215Device(BLE_ADDRESS, SECURITY_KEY, &handler, true, false, true, false);
  scanner.registerPTM215Device(BLE_ADDRESS, SECURITY_KEY, &handler2, false, true, true, true);
  log_d("Adding device Done");

}

void loop(){

    delay(1000);

} 