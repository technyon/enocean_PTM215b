#pragma once
/*
 * enocean_PTM215b.h
 *
 *  Created on: 16 dec. 2020
 *      Author: Jeroen
 */

#include "enocean_Constants.h"
#include "BLEDevice.h"
#include "BLEAdvertisedDevice.h"
#include "Arduino.h"
#include <map>
#include <vector>

// #define DEBUG_DATA
// #define DEBUG_COMMISSIONING_DATA
// #define DEBUG_ENCRYPTION
// #define DEBUG_REGISTER_CONFIG


// TODO Make configurable
#define INITIAL_REPEAT_WAIT            1000 
#define REPEAT_INTERVAL                 500

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
  Down ,
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
    Eventhandler() {};
    virtual ~Eventhandler() {};
    virtual void handleEvent(SwitchEvent& evt) = 0;
};

/**
 * @brief Implementation of an Enocean PTM215b powerless BLE switch 
 * 
 * The class works by starting a background taks which will scan for BLE advertising events
 * By deriving from BLEAdvertisedDeviceCallBack the class can be used in a ble scan task 
 * which will call Enocean_PTM251b.onResult() method when a message is received
 * 
 * On reception of an event from a registered switches the handleEvent() method of the event handler that is provided on construction will be called
 * 
 * The constructor has a boolean flag `enableRepeatTask` with which a second task can be launched that will
 * generate Repeat events every 500ms as long as a button is not released.
 * 
 * Before calling initialize() to create and start the tasks, the global BLEDevice must have been initialized with BLEDevice::init()
 */
class Enocean_PTM215b: public BLEAdvertisedDeviceCallbacks{
  public:
    Enocean_PTM215b(Eventhandler& handler, const boolean enableRepeatTask);
    virtual ~Enocean_PTM215b();

    /**
    * @brief Initialize object, init BLE and spiffs, open and read config file, start tasks
    */
    virtual void initialize();

    /**
    * @brief Register a switch
    * 
    * It can either be used to register 2 rocker switches (A and B) or 4 pushbutton switches (A0, A1, B0, B1)
    * 
    * @param bleAddress BLE address of switch being handled
    * @param securityKey Security key retreived from QR code, NFC or commissioning data 
    * @param nodeIdA Id of device that "belongs" to this switch A rocker (can be used in handleSwitchResult)
    * @param nodeIdB Id of device that "belongs" to this switch B rocker (can be used in handleSwitchResult)
    */
    void registerBleSwitch(const std::string bleAddress, const std::string securityKey, const uint8_t nodeIdA, const uint8_t nodeIdB);
    void registerBleSwitch(const std::string bleAddress, const std::string securityKey, const uint8_t nodeIdA0, const uint8_t nodeIdA1, 
                           const uint8_t nodeIdB0, const uint8_t nodeIdB1);

    /**
     * @brief Method used by repeatEventstask to generate a repeat event every 500ms
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
    /** Contents of a payload telegram */
    struct DataPayload{
        unsigned char len                   = 0x00;
        unsigned char type                  = 0x00;
        char manufacturerId[2] 	            = {0};
        uint32_t sequenceCounter            = 0;
        uint8_t switchStatus                = 0;
        // char optionalData[4] 	          = {0};  //TODO, make option to also read optional data when required
        char receivedSecurityKey[4]         = {0};
    };

    /** Contents of a commissioning telegram */
    struct CommissioningPayload{
        unsigned char len 				  = 0x00;
        unsigned char type 				  = 0x00;
        char manufacturerId[2] 		  = {0};
        char sequenceCounter[4] 	  = {0};
        char securityKey[16] 		    = {0};
        char staticSourceAddress[6] = {0};
    };

    struct Switch {
      uint32_t lastSequenceCounter = 0;
      uint8_t securityKey[16] = {0};
      uint8_t nodeIdA0;
      uint8_t nodeIdA1;
      uint8_t nodeIdB0;
      uint8_t nodeIdB1;
    };

    enum class ActionType {
      Release = 0,
      Press = 1,
    };    
    
    struct SwitchAction {
      uint8_t nodeId;
      Direction direction;
      ActionType actionType;
      uint32_t pushStartTime = 0;
    };
    
    Eventhandler& eventHandler;
    boolean enableRepeatTask;
    TaskHandle_t repeatEventsTaskHandle = nullptr;
    TaskHandle_t bleScanTaskHandle = nullptr;
    DataPayload dataPayloadBuffer;
    CommissioningPayload commissioningPayloadBuffer;

    /**
     * @brief Map of registered switches by BleAddress
     */
    std::map<std::string, Switch> switches;

    /**
     * @brief Map of Last events by NodeId
     * 
     */
    std::map<uint8_t, SwitchAction> actions;

    /**
    * @brief Create queue and start BLE scan task and switch handling task for detecting long press
    * 
    * @param -
    */
    void startTasks();
    
    /**
    * @brief overridden method from BLE to handle advertisement events. Checks manufacturer specific
    * data if it is an enocean switch
    * 
    * @param advertisedDevice Holds BLE address and payload
    */
    void onResult(BLEAdvertisedDevice advertisedDevice) override;
    
    /**
    * @brief Dedupes messages, Checks sequence counter to prevent replay attack,
    * and call handleSwitch action if security key is valid 
    * 
    * @param advertisedDevice Holds BLE address and payload
    */
    void handleDataPayload(std::string bleAddress);
    
    /**
    * @brief Handles commissioning data (to be implemented)
    * 
    * @param bleAddress BLE address of switch sending commissioning data
    */
    void handleCommissioningPayload(std::string bleAddress);

    /**
    * @brief Checks with AES128 encryption is sent security key is correct
    * 
    * @param bleAddress BLE address of switch being handled
    */
    bool securityKeyValid(std::string bleAddress);

    /**
    * @brief Handle final result of switch action
    * 
    * @param bleAddress BLE address of the switch
    * @param rocker Was rocker A0, A1, B0 or B1 pushed
    * @param switchResult Was rocker pushed long or short
    */
    // void handleSwitchResult(std::string bleAddress, uint8_t rocker, uint8_t switchResult);
    void handleSwitchAction(const uint8_t switchStatus, const std::string bleAddress);

};

} // namespace