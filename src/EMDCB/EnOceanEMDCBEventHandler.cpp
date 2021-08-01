#include "EnOceanEMDCBEventhandler.h"

namespace EnOcean {

EMDCBEventHandlerMap emdcbEventHandlerMap;

EMDCBEventHandler::EMDCBEventHandler(const uint8_t nodeId) : nodeId(nodeId) {
  emdcbEventHandlerMap[nodeId] = this;
}

EMDCBEventHandler::~EMDCBEventHandler() {
  emdcbEventHandlerMap.erase(nodeId);
}

} // namespace EnOcean
