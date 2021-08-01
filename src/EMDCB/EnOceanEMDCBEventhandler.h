#pragma once

#include "Arduino.h"
#include "EnOceanDataTypes.h"
#include "map"

namespace EnOcean {
  
enum class ParameterType : byte {
  LightLevel = 0x05,
  Occupancy = 0x20,
  BatteryVoltage = 0x01,
  EnergyLevel = 0x02,
  LightLevelSolar = 0x04
};

struct Parameter {
  ParameterType type;
  uint32_t value;
};

struct EMDCBEvent {
  std::vector<Parameter> parameters;
  Device* device;
};


/**
 * @brief Abstract class for handling EMDCB events, handleEvent() methods needs to be implemented on derived classes
 * 
 * On construction of a class derived from this, the instance is automatically registered in the global ptm215EventHandlerMap by nodeId
 * This allows registering an eventhandler to the EnOceanBLEScanner by nodeId, i.e. from a config file or from a setting stored in non-volatile storage
 * 
 * On destruction the mapping is removed.
 * 
 */
class EMDCBEventHandler {
public:
  EMDCBEventHandler(const uint8_t nodeId);
  virtual ~EMDCBEventHandler();

  const uint8_t getId() const { return nodeId; };

  virtual void handleEvent(EMDCBEvent& event) = 0;

private:
  const uint8_t nodeId;
};

typedef std::map<uint8_t, EMDCBEventHandler*> EMDCBEventHandlerMap;
extern EMDCBEventHandlerMap emdcbEventHandlerMap;

} // namespace EnOcean
