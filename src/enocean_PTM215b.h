#pragma once
/*
 * enocean_PTM215b.h
 *
 *  Created on: 16 dec. 2020
 *      Author: Jeroen
 */

#include "enocean_PTM215b.h"
#include "BLEDevice.h"
#include "BLEAdvertisedDevice.h"
#include "enocean_PTM215bConstants.h"
#include "Arduino.h"
#include <map>
#include "SPIFFS.h"
#include "ArduinoJson.h"

// #define BLE_DEBUG

class Enocean_PTM215b: public BLEAdvertisedDeviceCallbacks{
  public:
    Enocean_PTM215b();
    virtual ~Enocean_PTM215b();

    QueueHandle_t enocean_PTM215bBleIsrFlagQueue;
    
    /**
    * @brief Initialize object, init BLE and spiffs, open and read config file, start tasks
    * 
    * @param -
    */
    virtual void initialize();
    
    /**
    * @brief Handle final result of switch action
    * 
    * @param bleAddress BLE address of the switch
    * @param rocker Was rocker A0, A1, B0 or B1 pushed
    * @param switchResult Was rocker pushed long or short
    */
    void handleSwitchResult(std::string bleAddress, uint8_t rocker, uint8_t switchResult);

    struct bleSwitch{
      uint8_t rockerType = 0;
      bool rockerA0Pushed = false;
      uint32_t rockerA0PushedStartTime = 0;
      uint8_t rockerA0PushedType = PUSHED_UNDEFINED;
      bool rockerA1Pushed = false;
      uint32_t rockerA1PushedStartTime = 0;
      uint8_t rockerA1PushedType = PUSHED_UNDEFINED;
      bool rockerB0Pushed = false;
      uint32_t rockerB0PushedStartTime = 0;
      uint8_t rockerB0PushedType = PUSHED_UNDEFINED;
      bool rockerB1Pushed = false;
      uint32_t rockerB1PushedStartTime = 0;
      uint8_t rockerB1PushedType = PUSHED_UNDEFINED;
      bool rockerANotificationPending = false;
      bool rockerBNotificationPending = false;
      uint32_t lastSequenceCounter = 0;
      char securityKey[16] = {0};
      uint8_t observerId = 0;
    };

    std::map<std::string, bleSwitch> bleSwitches;

  private:
    /** Contents of a payload telegram */
    struct dataPayload{
        char len[1] 			                  = {0};
        char type[1] 			                  = {0};
        char manufacturerId[2] 	            = {0};
        uint32_t sequenceCounter            = 0;
        uint8_t switchStatus                = 0;
        // char optionalData[4] 	          = {0};  //TODO, make option to also read optional data when required
        char receivedSecurityKey[4]         = {0};
    };

    /** Contents of a commissioning telegram */
    struct commissioningPayload{
        char len[1] 				        = {0};
        char type[1] 				        = {0};
        char manufacturerId[2] 		  = {0};
        char sequenceCounter[4] 	  = {0};
        char securityKey[16] 		    = {0};
        char staticSourceAddress[6] = {0};
    };

    TaskHandle_t TaskHandleEnocean_PTM215b;
    TaskHandle_t TaskHandleBleScan;
    BaseType_t xHigherPriorityTaskWoken;
    
    /**
    * @brief Create queue and start BLE scan task and switch handling task for detecting long press
    * 
    * @param -
    */
    void startEnocean_PTM215bBleXtask();
    
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
    * @brief Handles switch push and release actions
    * 
    * @param switchStatus Push or release of switch A0, A1, B0 or B1
    * @param bleAddress BLE address of switch being handled
    */
    void handleSwitchAction(uint8_t switchStatus, std::string bleAddress);
    
    /**
    * @brief Stores switch in map
    * 
    * @param bleAddress BLE address of switch being handled
    * @param rockerType Defines Rocker A or B
    * @param securityKey Security key retreived from QR code, NFC or commissioning data 
    * @param observerId Id of device that "belongs" to this switch (can be used in handleSwitchResult)
    */
    void registerBleSwitch(std::string bleAddress, uint8_t rockerType, std::string securityKey, uint8_t observerId);
    
    /**
    * @brief Convert std::string holding 1 byte (2chars) hex data 
    * 
    * @param stringInput input string with 32 chars (16 bytes) of hex data
    * @param charArrayOutput reference to char array where to store converted data
    * @param byteLength length of charArrayOutput
    */
    void hexStringToCharArray(std::string stringInput, char * charArrayOutput, uint8_t byteLength);

    char securityKey[16] = {0};

    dataPayload dataPayloadBuffer;
    commissioningPayload commissioningPayloadBuffer;

    dataPayload lastNewDataPayload;
    std::string lastAddress;

    DynamicJsonDocument readConfigFile(File& file);

    /**
    * @brief Parse JSON file and create nodes based on the config and hardcoded TerminalDefinitionsMap
    * 
    * @param file  JSON file to parse
    */
    void readSettingsFromJSON(File& file);

};
