#pragma once
#include "EnOceanDataTypes.h"
#include "EnOceanPTM215Eventhandler.h"
#include "bitset"
#include "map"


namespace EnOcean {

// TODO Make configurable
#define INITIAL_REPEAT_WAIT 1000
#define REPEAT_INTERVAL 500


class PTM215EventAdapter {
public:
  ~PTM215EventAdapter();

  void registerHandler(Device& device, PTM215EventHandler* hander, bool buttonA0, bool buttonA1, bool buttonB0, bool buttonB1);
  void registerHandler(Device& device, const uint8_t nodeId, bool buttonA0, bool buttonA1, bool buttonB0, bool buttonB1);
  void handlePayload(Device& device, Payload& payload);

  /**
   * @brief Method used by repeatEventstask to generate a repeat event every XXX ms
   *
   */
  void generateRepeatEvents();

private:
  struct HandlerRegistration {
    NimBLEAddress address;
    PTM215EventHandler* handler;
    std::bitset<4> buttons; // A0, A1, B0, B1
  };
  std::vector<HandlerRegistration> handlers;

  TaskHandle_t repeatEventsTaskHandle = nullptr;
  std::map<NimBLEAddress, PTM215Event> lastEvents;

  void handleSwitchAction(const uint8_t switchStatus, NimBLEAddress& bleAddress);
  PTM215Event mapToPTM215Event(Device& device, Payload& payload);
  void manageEventList(PTM215Event& event);
  void callEventHandlers(PTM215Event& event);
  void suspendRepeatTask();
  void startRepeatTask();
  bool isRepeatTaskSuspended();
};

} // namespace EnOcean
