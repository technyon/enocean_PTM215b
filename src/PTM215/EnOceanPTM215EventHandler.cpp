#include "EnOceanPTM215Eventhandler.h"

namespace EnOcean {

PTM215EventHandlerMap ptm215EventHandlerMap;

PTM215EventHandler::PTM215EventHandler(const uint8_t nodeId) : nodeId(nodeId) {
  ptm215EventHandlerMap[nodeId] = this;
}

PTM215EventHandler::~PTM215EventHandler() {
  ptm215EventHandlerMap.erase(nodeId);
}

} // namespace EnOcean
