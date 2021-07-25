#pragma once
#include "EnoceanDataTypes.h"
#include "map"

namespace Enocean {

// TODO Make configurable
#define INITIAL_REPEAT_WAIT 1000
#define REPEAT_INTERVAL 500


enum class EventType : byte {
  Pushed = 0,
  Repeat,
  ReleaseShort,
  ReleaseLong
};

/**
 * @brief Up or Down button pressed / released
 *
 */
enum class Direction : byte {
  Down,
  Up
};

/**
 * @brief Event send to PTM215 eventhandler
 *
 */
struct PTM215Event {
  uint8_t nodeId;
  Direction direction;
  EventType eventType;
  uint32_t pushStartTime = 0;
  Device* device;
};

class PTM215EventAdapter {
public:
  ~PTM215EventAdapter();

  void handlePayload(Device& device, Payload& payload);

    /**
   * @brief Method used by repeatEventstask to generate a repeat event every XXX ms
   *
   */
  void generateRepeatEvents();

private:
  TaskHandle_t repeatEventsTaskHandle = nullptr;
  std::map<NimBLEAddress, PTM215Event> lastEvents;

  bool securityKeyValid(Device& device, Payload& payload);
  void handleSwitchAction(const uint8_t switchStatus, NimBLEAddress& bleAddress);
  PTM215Event mapToPTM215Event(Device& device, Payload& payload);
  void manageEventList(PTM215Event& event);

  void suspendRepeatTask();
  void startRepeatTask();
  bool isRepeatTaskSuspended();
};
  
} // namespace Enocean
