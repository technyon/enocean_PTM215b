#include "EnOceanSTM550BEventAdapter.h"
#include "esp_task_wdt.h"
#include "mbedtls/aes.h"
#include "EnOceanUtils.h"

namespace EnOcean {


STM550BEventAdapter::~STM550BEventAdapter() {
}

void STM550BEventAdapter::registerHandler(Device& device, const uint8_t nodeId) {
  if (stm550bEventHandlerMap.count(nodeId)) {
    registerHandler(device, stm550bEventHandlerMap[nodeId]);
  } else {
    log_e("NodeId [%d] not found in stm550bEventHandlerMap", nodeId);
  }
}

void STM550BEventAdapter::registerHandler(Device& device, STM550BEventHandler* handler) {
  HandlerRegistration reg;
  reg.address    = device.address;
  reg.handler    = handler;
  handlers.push_back(reg);
}

void STM550BEventAdapter::handlePayload(Device& device, Payload& payload) {
  STM550BEvent event = mapToSTM550BEvent(device, payload);
  callEventHandlers(event);
}

void STM550BEventAdapter::callEventHandlers(STM550BEvent& event) {
  for (auto const& reg : handlers) {
    if (reg.address == event.device->address) {
      reg.handler->handleEvent(event);
    }
  }
}

STM550BEvent STM550BEventAdapter::mapToSTM550BEvent(Device& device, Payload& payload) {
  STM550BEvent event;

  event.parameters = parsePayloadParameters(payload.data.raw, payload.len - 11);
  event.device = &device;

  return event;
}

} // namespace EnOcean
