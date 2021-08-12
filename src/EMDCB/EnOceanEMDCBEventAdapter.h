#pragma once
#include "../EnOceanDataTypes.h"
#include "EnOceanEMDCBEventhandler.h"
#include "bitset"
#include "map"


namespace EnOcean {

class EMDCBEventAdapter {
public:
  ~EMDCBEventAdapter();

  void registerHandler(Device& device, EMDCBEventHandler* hander);
  void registerHandler(Device& device, const uint8_t nodeId);
  void handlePayload(Device& device, Payload& payload);

private:
  struct HandlerRegistration {
    NimBLEAddress address;
    EMDCBEventHandler* handler;
  };
  std::vector<HandlerRegistration> handlers;

  void handleSwitchAction(const uint8_t switchStatus, NimBLEAddress& bleAddress);
  EMDCBEvent mapToEMDCBEvent(Device& device, Payload& payload);
  void callEventHandlers(EMDCBEvent& event);
};

} // namespace EnOcean
