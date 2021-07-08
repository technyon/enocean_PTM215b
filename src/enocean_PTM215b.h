#pragma once
/*
 * enocean_PTM215b.h
 *
 *  Created on: 16 dec. 2020
 *      Author: Jeroen
 */

#include "Arduino.h"
#include "NimBLEDevice.h"
#include "enocean_Constants.h"
#include <map>
#include <vector>

// #define DEBUG_DATA
// #define DEBUG_COMMISSIONING_DATA
// #define DEBUG_ENCRYPTION
// #define DEBUG_REGISTER_CONFIG

// TODO Make configurable
#define INITIAL_REPEAT_WAIT 1000
#define REPEAT_INTERVAL 100

namespace PTM215b {

enum class EventType {
  Pushed,
  Repeat,
  ReleaseShort,
  ReleaseLong
};

/**
 * @brief Up or Down button pressed / released
 *
 */
enum class Direction { 
  Down,
  Up 
};

/**
 * @brief Event send to registered eventhandler
 *
 */
struct SwitchEvent {
  uint8_t nodeId;
  Direction direction;
  EventType eventType;
};

class Eventhandler {
public:
  Eventhandler(){};
  virtual ~Eventhandler(){};
  virtual void handleEvent(SwitchEvent& evt) = 0;
};

  enum PayloadType {
    Data,
    Commisioning
  };
  struct Payload {
    unsigned char len;
    unsigned char type;
    char manufacturerId[2];
    uint32_t sequenceCounter;
    PayloadType payloadType;
    union {
      struct { 
        uint8_t switchStatus;
        char optionalData[4];
        char receivedSecurityKey[4];
      } data;
      struct { 
        char securityKey[16];
        char staticSourceAddress[6];
      } commisioning;
    };
  };

/**
 * @brief Class handling BLE advertisement messages received from multiple
 * Enocean BLE215b devices
 *
 * The class works by starting a background taks which will scan for BLE
 * advertising events By deriving from BLEAdvertisedDeviceCallBack the class can
 * be used in a ble scan task which will call Enocean_PTM251b.onResult() method
 * when a message is received
 *
 * PTM15B switches need to be registered to this class using registerBleSwitch,
 * which links the BLE address to a event handler which will be called on
 * reception of an event on that BLE address
 *
 * The constructor has a boolean flag `enableRepeatTask` with which a second
 * task can be launched that will generate Repeat events every 500ms as long as
 * a button is not released.
 *
 * Before calling initialize() to create and start the tasks, the global
 * BLEDevice must have been initialized with BLEDevice::init()
 */
class Enocean_PTM215b : public BLEAdvertisedDeviceCallbacks {
public:
  Enocean_PTM215b(const boolean enableRepeatTask);
  virtual ~Enocean_PTM215b();

  /**
   * @brief Set task priorities
   *
   */
  void setScanTaskPriority(uint8_t prio);
  void setRepeatTaskPriority(uint8_t prio);

  /**
   * @brief return is task is suspended
   *
   * @param taskHandle taskHandle to check
   */
  bool isSuspended(TaskHandle_t taskHandle);

  /**
   * @brief Initialize object and start background tasks
   *
   * The BLEDevice must have been initialized, otherwise an error will be logged
   * and the background will not be started This must be called before using the
   * switches
   */
  void initialize();

  /**
   * @brief Register a switch
   *
   * It can either be used to register 2 rocker switches (A and B) or 4
   * pushbutton switches (A0, A1, B0, B1)
   *
   * @param bleAddress BLE address of switch being handled
   * @param securityKey Security key retreived from QR code, NFC or
   * commissioning data
   * @param nodeIdA Id of device that "belongs" to this switch A rocker (can be
   * used in handleSwitchResult)
   * @param nodeIdB Id of device that "belongs" to this switch B rocker (can be
   * used in handleSwitchResult)
   * @param handler Pointer to an EventHandler that will be called on reception
   * of events from this switch
   */
  void registerBleSwitch(const std::string bleAddress,
                         const std::string securityKey, const uint8_t nodeIdA,
                         const uint8_t nodeIdB, Eventhandler* handler);
  void registerBleSwitch(const std::string bleAddress,
                         const std::string securityKey, const uint8_t nodeIdA0,
                         const uint8_t nodeIdA1, const uint8_t nodeIdB0,
                         const uint8_t nodeIdB1, Eventhandler* handler);

  /**
   * @brief Method used by repeatEventstask to generate a repeat event every XXX
   * ms
   *
   */
  void generateRepeatEvents();

  /**
   * @brief Returns the number of switches registered
   *
   * @return uint8_t
   */
  uint8_t registeredSwitchCount() { return switches.size(); }

private:

  struct Switch {
    uint32_t lastSequenceCounter = 0;
    uint8_t securityKey[16]      = {0};
    uint8_t nodeIdA0;
    uint8_t nodeIdA1;
    uint8_t nodeIdB0;
    uint8_t nodeIdB1;
    Eventhandler* eventHandler;
  };

  enum class ActionType {
    Release = 0,
    Press   = 1,
  };

  struct SwitchAction {
    uint8_t nodeId;
    Direction direction;
    ActionType actionType;
    uint32_t pushStartTime = 0;
    Eventhandler* eventHandler;
  };

  boolean enableRepeatTask;
  TaskHandle_t repeatEventsTaskHandle = nullptr;
  TaskHandle_t bleScanTaskHandle      = nullptr;

  /**
   * @brief Map of registered switches by BleAddress
   */
  std::map<BLEAddress, Switch> switches;

  /**
   * @brief Map of Last events by NodeId
   *
   */
  std::map<uint8_t, SwitchAction> actions;

  /**
   * @brief Create queue and start BLE scan task and switch handling task for
   * detecting long press
   *
   * @param -
   */
  void startTasks();

  /**
   * @brief Suspends/resumes the repeat task used for detecting llong press
   *
   * @param suspend true = suspend, false = resume
   */
  void suspendRepeatTask(bool suspend);

  /**
   * @brief overridden method from BLE to handle advertisement events. Checks
   * manufacturer specific data if it is an enocean switch
   *
   * @param advertisedDevice Holds BLE address and payload
   */
  void onResult(BLEAdvertisedDevice* advertisedDevice) override;
  
  Payload getPayload(NimBLEAdvertisedDevice* advertisedDevice);

  /**
   * @brief Dedupes messages, Checks sequence counter to prevent replay attack,
   * and call handleSwitch action if security key is valid
   *
   * @param advertisedDevice Holds BLE address and payload
   * @param payload
   */
  void handleDataPayload(BLEAddress& bleAddress, Payload& payload);

  /**
   * @brief Handles commissioning data (to be implemented)
   *
   * @param bleAddress BLE address of switch sending commissioning data
   * @param payload
   */
  void handleCommissioningPayload(BLEAddress& bleAddress, Payload& payload);

  /**
   * @brief Checks with AES128 encryption is sent security key is correct
   *
   * @param bleAddress BLE address of switch being handled
   */
  bool securityKeyValid(BLEAddress& bleAddress, Payload& payload);

  /**
   * @brief Handle final result of switch action
   *
   * @param switchStatus Was rocker pushed long or short
   * @param bleAddress BLE address of the switch
   */
  void handleSwitchAction(const uint8_t switchStatus, BLEAddress& bleAddress);
};

} // namespace PTM215b