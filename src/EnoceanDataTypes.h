#pragma once
#include "Arduino.h"
#include "NimBLEAddress.h"

namespace EnOcean {

enum class PayloadType {
  Data,
  Commissioning
};

enum class DeviceType {
  UNKNOWN,
  PTM215B,    // Switches
  PTM535BZ,   // ??
  EMDCB,      // Motion detector
  STM550B     // Multi sensor
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

struct Device {
  NimBLEAddress address;
  uint8_t securityKey[16]      = {0};
  uint32_t lastSequenceCounter = 0;
  DeviceType type;
};

enum class ParameterType : byte {
  Temperature = 0x00,
  BatteryVoltage = 0x01,
  EnergyLevel = 0x02,
  LightLevelSolar = 0x04,
  LightLevel = 0x05,
  Humidity = 0x06,
  Acceleration = 0x0A,
  Occupancy = 0x20,
  MagnetContact = 0x23
};

// TODO Handle custom length parameters
struct Parameter {
  ParameterType type;
  uint8_t size;
  union {
    uint32_t uint32;
    uint16_t uint16;
    uint8_t uint8;
  } value { 0x00 };
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

} // namespace EnOcean