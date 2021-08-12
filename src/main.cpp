#include "Arduino.h"
#include "EnOceanBLEScanner.h"
#include "EMDCB/EnOceanEMDCBEventAdapter.h"
#include "STM550B/EnOceanSTM550BEventAdapter.h"
#include "EnOceanUtils.h"

#define PTM_BLE_ADDRESS "E2:15:00:00:D4:AD"
#define PTM_SECURITY_KEY "8B49C1B3C62DAE598DCFD4C1AFB1956A"

#define EMDCB_BLE_ADDRESS "E5:00:00:00:0C:79"
#define EMDCB_SECURITY_KEY "31262A904F6D7B4E02DC637A33E762BF"

#define STM550_BLE_ADDRESS "E5:00:10:00:15:D0"
#define STM550_SECURITY_KEY "F58AFD8719BAB1600B2CCCD06FAE1A98"

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
    log_i("\tType \tSize Value");
    log_i("-----------------------------");
    for (auto const& parameter : event.parameters) {
      char b[5];
      log_i("\t0x%02x \t   %d %s", parameter.type, parameter.size, EnOcean::printBuffer((byte*)&parameter.value, parameter.size, false, "", b));  
    }
  }
};

class STM550Handler : public EnOcean::STM550BEventHandler {
public:
  STM550Handler(const uint8_t id) : EnOcean::STM550BEventHandler(id) {};
  virtual ~STM550Handler() {};

  void handleEvent(EnOcean::STM550BEvent& event) override {
    log_i("Handling STM550B Event by node %d", getId());
    log_i("DeviceAddress: %s", event.device->address.toString().c_str());
    log_i("Parameters:");
    log_i("\tType \tSize Value");
    log_i("-----------------------------");
    for (auto const& parameter : event.parameters) {
      char b[5];
      log_i("\t0x%02x \t   %d %s", parameter.type, parameter.size, EnOcean::printBuffer((byte*)&parameter.value, parameter.size, false, "", b));  
    }
  }
};

EnOcean::BLEScanner* scanner;

void testEMDCBSignature() {
  EnOcean::EMDCBEventAdapter adapter;

  EnOcean::Device device;
  device.address = NimBLEAddress("E5:00:00:00:00:C4");
  uint8_t key[] {0x9E, 0x0D, 0xE9, 0xC2, 0x53, 0x86, 0xB6, 0xC4, 0xF0, 0x70, 0x64, 0x2E, 0x19, 0xE0, 0x36, 0x80};
  memcpy(device.securityKey, key, 16);
  EnOcean::Payload payload;
  payload.len = 0x15;
  payload.type = 0xFF;
  payload.payloadType = EnOcean::PayloadType::Data;
  payload.manufacturerId[0] = 0xDA;
  payload.manufacturerId[1] = 0x03;
  payload.sequenceCounter = 0x0001E257;
  uint8_t data[] {0x02, 0xaa, 0x44, 0xd6, 0x00, 0x45, 0x35, 0x00, 0x20, 0x02};
  memset(payload.data.raw, 0x00, sizeof(payload.data.raw));
  memcpy(payload.data.raw, data, sizeof(data));

  uint8_t signature[] {0xc8, 0xcc, 0x57, 0x12};
  memcpy(payload.data.signature, signature, 4);

  // EnOcean::printBuffer((byte*)&payload, sizeof(payload), false, "Test Payload");
  log_i("EMDCB Test Security key %s", scanner->securityKeyValid(device, payload) ? "valid" : "NOT valid");
}

void testSTM550Signature() {
  EnOcean::STM550BEventAdapter adapter;

  EnOcean::Device device;
  device.address = NimBLEAddress("E5:00:77:01:00:00");
  uint8_t key[] {0x4D, 0xB3, 0x4D, 0xB3, 0x07, 0x0E, 0xFC, 0x67, 0x13, 0xFE, 0x39, 0xE1, 0x3C, 0xF3, 0xC9, 0x39};
  memcpy(device.securityKey, key, 16);
  EnOcean::Payload payload;
  payload.len = 0x1C;
  payload.type = 0xFF;
  payload.payloadType = EnOcean::PayloadType::Data;
  payload.manufacturerId[0] = 0xDA;
  payload.manufacturerId[1] = 0x03;
  payload.sequenceCounter = 0x00027FB;
  uint8_t data[] {0x02, 0x68, 0x45, 0x8D, 0x01, 0x40, 0x42, 0x09, 0x06, 0x57, 0x8A, 0xF7, 0x91, 0xE6, 0x5E, 0x23, 0x01 };
  memset(payload.data.raw, 0x00, sizeof(payload.data.raw));
  memcpy(payload.data.raw, data, sizeof(data));

  uint8_t signature[] {0x0F, 0x01, 0x57, 0xD3};
  memcpy(payload.data.signature, signature, 4);

  // EnOcean::printBuffer((byte*)&payload, sizeof(payload), false, "Test Payload");
  log_i("STM550 Test Security key %s", scanner->securityKeyValid(device, payload) ? "valid" : "NOT valid");
}

void testPTMSignature() {
  EnOcean::Device device;
  EnOcean::PTM215EventAdapter ptmAdapter;

  device.address = NimBLEAddress("E2:15:00:00:19:B8");
  uint8_t key2[] {0x3D, 0xDA, 0x31, 0xAD, 0x44, 0x76, 0x7A, 0xE3, 0xCE, 0x56, 0xDC, 0xE2, 0xB3, 0xCE, 0x2A, 0xBB};
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

  // EnOcean::printBuffer((byte*)&payload, sizeof(payload), false, "Payload");

  log_i("PTM215 Test Security key %s", scanner->securityKeyValid(device, payload) ? "valid" : "NOT valid");
}

PTMHandler* handler1;
PTMHandler* handler2;
EMDCBHandler* emdcbHandler;
STM550Handler* stmHandler;

void setup() {
  Serial.begin(115200);
  log_i("Starting EnOcean BLE Example application...");

  scanner = new EnOcean::BLEScanner();
  handler1 = new PTMHandler(1);
  handler2 = new PTMHandler(2);
  emdcbHandler = new EMDCBHandler(3);
  stmHandler = new STM550Handler(4);

  NimBLEDevice::init("ESP32_client");

  scanner->initialize();

  log_d("Adding devices");
  // register handler for A0 and B0 buttons using pointer to handler
  scanner->registerPTM215Device(PTM_BLE_ADDRESS, PTM_SECURITY_KEY, handler1, true, false, true, false);
  // register handler for A1, B0 and B1 buttons, using nodeId of handler
  scanner->registerPTM215Device(PTM_BLE_ADDRESS, PTM_SECURITY_KEY, 2, false, true, true, true);

  scanner->registerEMDCBDevice(EMDCB_BLE_ADDRESS, EMDCB_SECURITY_KEY, emdcbHandler);

  scanner->registerSTM550BDevice(STM550_BLE_ADDRESS, STM550_SECURITY_KEY, stmHandler);
  log_i("Initialization done");
  log_i("===========================================");

  testPTMSignature();
  testEMDCBSignature();
  testSTM550Signature();
}

void loop() {
  // Nothing to do here
  delay(1000);
}