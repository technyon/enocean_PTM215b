#include "BLEScanner.h"

#define BLE_ADDRESS "E2:15:00:00:D4:AD"
#define SECURITY_KEY "8B49C1B3C62DAE598DCFD4C1AFB1956A"

class Handler : public Enocean::PayloadHandler {
public:
  Handler() {};

  void handlePayload(Enocean::Payload& payload) override {
    log_d("Handling Payload");
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