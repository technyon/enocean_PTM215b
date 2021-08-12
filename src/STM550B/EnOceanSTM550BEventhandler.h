#pragma once

#include "Arduino.h"
#include "EnOceanDataTypes.h"
#include "map"

namespace EnOcean {

struct STM550BEvent {
  std::vector<Parameter> parameters;
  Device* device;
};

/**
 * @brief Abstract class for handling STM550B events, handleEvent() methods needs to be implemented on derived classes
 * 
 * On construction of a class derived from this, the instance is automatically registered in the global ptm215EventHandlerMap by nodeId
 * This allows registering an eventhandler to the EnOceanBLEScanner by nodeId, i.e. from a config file or from a setting stored in non-volatile storage
 * 
 * On destruction the mapping is removed.
 * 
 */
class STM550BEventHandler {
public:
  STM550BEventHandler(const uint8_t nodeId);
  virtual ~STM550BEventHandler();

  const uint8_t getId() const { return nodeId; };

  virtual void handleEvent(STM550BEvent& event) = 0;

private:
  const uint8_t nodeId;
};

typedef std::map<uint8_t, STM550BEventHandler*> STM550BEventHandlerMap;
extern STM550BEventHandlerMap stm550bEventHandlerMap;

} // namespace EnOcean
