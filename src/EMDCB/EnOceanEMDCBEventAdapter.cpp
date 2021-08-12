#include "EnOceanEMDCBEventAdapter.h"
#include "EnOceanUtils.h"
#include "esp_task_wdt.h"
#include "mbedtls/aes.h"


namespace EnOcean {

EMDCBEventAdapter::~EMDCBEventAdapter() {
}

void EMDCBEventAdapter::registerHandler(Device& device, const uint8_t nodeId) {
  if (emdcbEventHandlerMap.count(nodeId)) {
    registerHandler(device, emdcbEventHandlerMap[nodeId]);
  } else {
    log_e("NodeId [%d] not found in emdcbEventHandlerMap", nodeId);
  }
}

void EMDCBEventAdapter::registerHandler(Device& device, EMDCBEventHandler* handler) {
  HandlerRegistration reg;
  reg.address = device.address;
  reg.handler = handler;
  handlers.push_back(reg);
}

void EMDCBEventAdapter::handlePayload(Device& device, Payload& payload) {
  EMDCBEvent event = mapToEMDCBEvent(device, payload);
  callEventHandlers(event);
}

void EMDCBEventAdapter::callEventHandlers(EMDCBEvent& event) {
  for (auto const& reg : handlers) {
    if (reg.address == event.device->address) {
      reg.handler->handleEvent(event);
    }
  }
}

EMDCBEvent EMDCBEventAdapter::mapToEMDCBEvent(Device& device, Payload& payload) {
  EMDCBEvent event;

  event.parameters = parsePayloadParameters(payload.data.raw, payload.len - 11);
  event.device     = &device;

  return event;
}

} // namespace EnOcean
