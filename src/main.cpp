#include "EnOceanBLEScanner.h"
#include "EMDCB/EnOceanEMDCBEventAdapter.h"
#include "EnOceanUtils.h"

#define PTM_BLE_ADDRESS "E2:15:00:00:D4:AD"
#define PTM_SECURITY_KEY "8B49C1B3C62DAE598DCFD4C1AFB1956A"

#define EMDCB_BLE_ADDRESS "E5:00:00:00:0C:79"
#define EMDCB_SECURITY_KEY "31262A904F6D7B4E02DC637A33E762BF"

class PTMHandler : public EnOcean::PTM215EventHandler {
public:
  PTMHandler(const uint8_t id) : EnOcean::PTM215EventHandler(id) {};
  virtual ~PTMHandler() {};

  void handleEvent(EnOcean::PTM215Event& event) override {
    log_i("Handling PTM Event by node %d", getId());
    log_i("DeviceAddress: %s", event.device->address.toString().c_str());
    log_i("Event: button %d type %d", event.button, event.eventType);
  }
};

class EMDCBHandler : public EnOcean::EMDCBEventHandler {
public:
  EMDCBHandler(const uint8_t id) : EnOcean::EMDCBEventHandler(id) {};
  virtual ~EMDCBHandler() {};

  void handleEvent(EnOcean::EMDCBEvent& event) override {
    log_i("Handling EMDCB Event by node %d", getId());
    log_i("DeviceAddress: %s", event.device->address.toString().c_str());
    log_i("Parameters:");
    log_i("\tType \tSize \tValue");
    log_i("-----------------------------");
    for (auto const& parameter : event.parameters) {
      char b[5];
      log_i("\t0x%02x \t%d \t%s", parameter.type, parameter.size, EnOcean::printBuffer((byte*)&parameter.value, parameter.size, false, "", b));  
    }
  }
};

void testEMDCBSignature() {
  EnOcean::EMDCBEventAdapter adapter;

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
  memset(payload.data.raw, 0x00, sizeof(payload.data.raw));
  memcpy(payload.data.raw, data, sizeof(data));

  uint8_t signature[] = {0xc8, 0xcc, 0x57, 0x12};
  memcpy(payload.data.signature, signature, 4);

  EnOcean::printBuffer((byte*)&payload, sizeof(payload), false, "Test Payload");
  log_i("EMDCB Security key %s", adapter.securityKeyValid(device, payload) ? "valid" : "NOT valid");
}

void testPTMSignature() {
  EnOcean::Device device;
  EnOcean::PTM215EventAdapter ptmAdapter;

  device.address = NimBLEAddress("E2:15:00:00:19:B8");
  uint8_t key2[] = {0x3D, 0xDA, 0x31, 0xAD, 0x44, 0x76, 0x7A, 0xE3, 0xCE, 0x56, 0xDC, 0xE2, 0xB3, 0xCE, 0x2A, 0xBB};
  memcpy(device.securityKey, key2, 16);
  EnOcean::Payload payload;
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

  log_i("PTM215 Security key %s", ptmAdapter.securityKeyValid(device, payload) ? "valid" : "NOT valid");
}

EnOcean::BLEScanner* scanner;
PTMHandler* handler1;
PTMHandler* handler2;
EMDCBHandler* emdcbHandler;

void setup() {
  Serial.begin(115200);
  log_i("Starting EnOcean BLE Example application...");

  scanner = new EnOcean::BLEScanner();
  handler1 = new PTMHandler(1);
  handler2 = new PTMHandler(2);
  emdcbHandler = new EMDCBHandler(3);

  NimBLEDevice::init("ESP32_client");

  scanner->initialize();

  log_d("Adding devices");
  // register handler for A0 and B0 buttons using pointer to handler
  scanner->registerPTM215Device(PTM_BLE_ADDRESS, PTM_SECURITY_KEY, handler1, true, false, true, false);
  // register handler for A1, B0 and B1 buttons, using nodeId of handler
  scanner->registerPTM215Device(PTM_BLE_ADDRESS, PTM_SECURITY_KEY, 2, false, true, true, true);

  scanner->registerEMDCBDevice(EMDCB_BLE_ADDRESS, EMDCB_SECURITY_KEY, emdcbHandler);
  log_i("Initialization done");
  log_i("===========================================");

  testEMDCBSignature();
}

void loop() {
  // Nothing to do here
  delay(1000);
}