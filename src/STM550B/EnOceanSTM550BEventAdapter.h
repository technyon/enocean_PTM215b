#pragma once
#include "../EnOceanDataTypes.h"
#include "EnOceanSTM550BEventhandler.h"
#include "bitset"
#include "map"


namespace EnOcean {

class STM550BEventAdapter {
public:
  ~STM550BEventAdapter();

  void registerHandler(Device& device, STM550BEventHandler* hander);
  void registerHandler(Device& device, const uint8_t nodeId);
  void handlePayload(Device& device, Payload& payload);

private:
  struct HandlerRegistration {
    NimBLEAddress address;
    STM550BEventHandler* handler;
  };
  std::vector<HandlerRegistration> handlers;

  void handleSwitchAction(const uint8_t switchStatus, NimBLEAddress& bleAddress);
  STM550BEvent mapToSTM550BEvent(Device& device, Payload& payload);
  void callEventHandlers(STM550BEvent& event);
};

} // namespace EnOcean
