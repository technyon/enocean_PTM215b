#include "EnOceanSTM550BEventhandler.h"

namespace EnOcean {

STM550BEventHandlerMap stm550bEventHandlerMap;

STM550BEventHandler::STM550BEventHandler(const uint8_t nodeId) : nodeId(nodeId) {
  stm550bEventHandlerMap[nodeId] = this;
}

STM550BEventHandler::~STM550BEventHandler() {
  stm550bEventHandlerMap.erase(nodeId);
}

} // namespace EnOcean
