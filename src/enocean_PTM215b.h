#pragma once
/*
 * enocean_PTM215b.h
 *
 *  Created on: 16 dec. 2020
 *      Author: Jeroen
 */

#include "BLEDevice.h"
#include "BLEAdvertisedDevice.h"
#include "enocean_PTM215bConstants.h"
#include "Arduino.h"
#include <map>
#include <vector>

namespace PTM215b {

enum class Direction {
  Up, Down
};

enum class EventType {
  Pushed,
  Repeat,
  Released
};

struct BleSwitch {
  uint32_t lastSequenceCounter;
  char securityKey[16] = {0};
  uint8_t nodeIdA;
  uint8_t nodeIdB;
};

struct BleSwitchEvent {
  uint8_t nodeId;
  Direction direction;
  EventType eventType;
  uint32_t pushStartTime;
};

class Eventhandler {
public:
    Eventhandler() {};
    virtual ~Eventhandler() {};

    virtual void handleEvent(BleSwitchEvent& evt) = 0;

};


class Enocean_PTM215b: public BLEAdvertisedDeviceCallbacks{
  public:
    Enocean_PTM215b(Eventhandler& handler);
    virtual ~Enocean_PTM215b();

    /**
    * @brief Initialize object, init BLE and spiffs, open and read config file, start tasks
    */
    virtual void initialize();

    /**
    * @brief Register a switch
    * 
    * @param bleAddress BLE address of switch being handled
    * @param securityKey Security key retreived from QR code, NFC or commissioning data 
    * @param nodeIdA Id of device that "belongs" to this switch A rocker (can be used in handleSwitchResult)
    * @param nodeIdB Id of device that "belongs" to this switch B rocker (can be used in handleSwitchResult)
    */
    void registerBleSwitch(const std::string bleAddress, const std::string securityKey, const uint8_t nodeIdA, const uint8_t nodeIdB);

    void generateRepeatEvents();
    uint8_t registeredSwitchCount() { return bleSwitches.size(); }
    
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
    
    TaskHandle_t repeatEventsTaskHandle;
    TaskHandle_t bleScanTaskHandle;
    Eventhandler& eventHandler;
    DataPayload dataPayloadBuffer;
    CommissioningPayload commissioningPayloadBuffer;

    /**
     * @brief Map of registered switches by BleAddress
     */
    std::map<std::string, BleSwitch> bleSwitches;

    /**
     * @brief Map of Last events by NodeId
     * 
     */
    std::map<uint8_t, BleSwitchEvent> events;

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
    
  
    /**
    * @brief Convert std::string holding 1 byte (2chars) hex data 
    * 
    * @param stringInput input string with 32 chars (16 bytes) of hex data
    * @param charArrayOutput reference to char array where to store converted data
    * @param byteLength length of charArrayOutput
    */
    void hexStringToCharArray(std::string stringInput, char * charArrayOutput, uint8_t byteLength);

};

} // namespace