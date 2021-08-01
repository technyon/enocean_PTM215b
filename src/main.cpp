#include "EnOceanBLEScanner.h"
#include "EMDCB/EnOceanEMDCBEventAdapter.h"
#include "EnOceanUtils.h"

#define BLE_ADDRESS "E2:15:00:00:D4:AD"
#define SECURITY_KEY "8B49C1B3C62DAE598DCFD4C1AFB1956A"

class Handler : public EnOcean::PTM215EventHandler {
public:
  Handler(const uint8_t id) : EnOcean::PTM215EventHandler(id) {};
  virtual ~Handler() {};

  void handleEvent(EnOcean::PTM215Event& event) override {
    log_i("Handling Event by node %d", getId());
    log_i("DeviceAddress: %s", event.device->address.toString().c_str());
    log_i("Event: button %d type %d", event.button, event.eventType);
  }

};

EnOcean::BLEScanner* scanner;
Handler* handler1;
Handler* handler2;

void setup() {
  Serial.begin(115200);
  log_i("Starting EnOcean BLE Example application...");

  scanner = new EnOcean::BLEScanner();
  handler1 = new Handler(1);
  handler2 = new Handler(2);

  NimBLEDevice::init("ESP32_client");

  scanner->initialize();

  log_d("Adding devices");
  // register handler for A0 and B0 buttons using pointer to handler
  scanner->registerPTM215Device(BLE_ADDRESS, SECURITY_KEY, handler1, true, false, true, false);
  // register handler for A1, B0 and B1 buttons, using nodeId of handler
  scanner->registerPTM215Device(BLE_ADDRESS, SECURITY_KEY, 2, false, true, true, true);
  log_i("Initialization done");
  log_i("===========================================");

  EnOcean::EMDCBEventAdapter adapter;
  EnOcean::PTM215EventAdapter ptmadapter;

  EnOcean::Device device;
  device.address = NimBLEAddress("E5:00:00:00:00:C4");
  uint8_t key[] = {0x9E, 0x0D, 0xE9, 0xC2, 0x53, 0x86, 0xB6, 0xC4, 0xF0, 0x70, 0x64, 0x2E, 0x19, 0xE0, 0x36, 0x80};
  memcpy(device.securityKey, key, 16);
  EnOcean::Payload payload;
  payload.len = 0x15;
  payload.type = 0xFF;
  payload.payloadType = EnOcean::PayloadType::Data;
  payload.manufacturerId[0] = 0xDA;
  payload.manufacturerId[1] = 0x03;
  payload.sequenceCounter = 0x0001E257;
  uint8_t data[] = {0x02, 0xaa, 0x44, 0xd6, 0x00, 0x45, 0x35, 0x00, 0x20, 0x02};
  memcpy(payload.data.raw, data, sizeof(data));

  uint8_t signature[] = {0xc8, 0xcc, 0x57, 0x12};
  memcpy(payload.data.signature, signature, 4);

  EnOcean::printBuffer((byte*)&payload, sizeof(payload), false, "Payload");

  log_i("EMDCB Security key %s", adapter.securityKeyValid(device, payload) ? "valid" : "NOT valid");

  device.address = NimBLEAddress("E2:15:00:00:19:B8");
  uint8_t key2[] = {0x3D, 0xDA, 0x31, 0xAD, 0x44, 0x76, 0x7A, 0xE3, 0xCE, 0x56, 0xDC, 0xE2, 0xB3, 0xCE, 0x2A, 0xBB};
  memcpy(device.securityKey, key2, 16);
  memset(&payload, 0x00, sizeof(payload));
  payload.len = 0x0C;
  payload.type = 0xFF;
  payload.payloadType = EnOcean::PayloadType::Data;
  payload.manufacturerId[0] = 0xDA;
  payload.manufacturerId[1] = 0x03;
  payload.sequenceCounter = 0x0000045D;
  uint8_t data2[] = {0x11};
  memcpy(payload.data.raw, data2, sizeof(data2));

  uint8_t signature2[] = {0xB2, 0xFA, 0x88, 0xFF};
  memcpy(payload.data.signature, signature2, 4);

  EnOcean::printBuffer((byte*)&payload, sizeof(payload), false, "Payload");

  log_i("PTM215 Security key %s", adapter.securityKeyValid(device, payload) ? "valid" : "NOT valid");
  log_i("PTM215 Security key %s", ptmadapter.securityKeyValid(device, payload) ? "valid" : "NOT valid");

}

void loop() {
  // Nothing to do here
  delay(1000);
}