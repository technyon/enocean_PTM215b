#pragma once
#include "Arduino.h"
#include "NimBLEAddress.h"

namespace Enocean {

enum class PayloadType {
  Data,
  Commissioning
};

enum class DeviceType {
  UNKNOWN,
  PTM215B,
  PTM535BZ,
  EMDCB,
  STM550B
};

struct Payload {
  byte len;
  byte type;
  char manufacturerId[2];
  uint32_t sequenceCounter;
  DeviceType deviceType;
  PayloadType payloadType;
  union {
    struct { // Data
      byte raw[19];
      byte signature[4];
    } data;
    struct { // Commissioning
      byte securityKey[16];
      byte staticSourceAddress[6]; // LSB first
    } commissioning;
  };
};

class PayloadHandler {
public:
  PayloadHandler(){};
  virtual ~PayloadHandler(){};
  virtual void handleEvent(DeviceType type, void* event) = 0; //untyped for now, need to handle events from different devices
};

struct Device {
  NimBLEAddress address;
  uint8_t securityKey[16]      = {0};
  uint32_t lastSequenceCounter = 0;
  DeviceType type;
  PayloadHandler* handler;
};

struct CommissioningEvent {
  NimBLEAddress address;
  DeviceType type;
  byte securityKey[17];
};

/**
 * @brief Handler for commissionEvents from a BLE Switch
 * 
 * The event is sent when the swtich is put into commissioning mode and when the same
 * button is pressed or released when in commissioning mode.
 * 
 * Note that the handler must be able to handle receiving the same commission event multiple times!
 */
class CommissioningEventhandler {
public:
  CommissioningEventhandler(){};
  virtual ~CommissioningEventhandler(){};
  virtual void handleEvent(CommissioningEvent& evt) = 0;
};

} // namespace Enocean