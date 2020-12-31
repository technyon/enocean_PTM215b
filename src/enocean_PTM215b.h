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

// #define BLE_DEBUG

class Enocean_PTM215b: public BLEAdvertisedDeviceCallbacks{
  public:
    Enocean_PTM215b();
    virtual ~Enocean_PTM215b();

    QueueHandle_t enocean_PTM215bBleIsrFlagQueue;

    virtual void initialize();

    struct bleSwitch{
      uint8_t switchId = 0;
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
    };

    std::map<std::string, bleSwitch> bleSwitches;

  private:
    /** Contents of a payload telegram */
    struct dataPayload{
        char len[1] 			        = {0};
        char type[1] 			        = {0};
        char manufacturerId[2] 	  = {0};
        uint32_t sequenceCounter  = 0;
        uint8_t switchStatus      = 0;
        char optionalData[4] 	    = {0};
        char securityKey[4]   	  = {0};
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
    void startEnocean_PTM215bBleXtask();
    void pushNotificationToQueue();
    void onResult(BLEAdvertisedDevice advertisedDevice) override;
    void handleDataPayload(std::string bleAddress);
    void handleCommissioningPayload(std::string bleAddress);

    void handleSwitchAction(uint8_t switchStatus, std::string bleAddress);
    void handleSwitchResult(std::string bleAddress, uint8_t rocker, uint8_t switchResult);
    void registerBleSwitch(std::string bleAddress, uint8_t switchId);

    char securityKey[16] = {0};
    esp_bd_addr_t scannedBleAddress;

    dataPayload dataPayloadBuffer;
    commissioningPayload commissioningPayloadBuffer;

    dataPayload lastNewDataPayload;
    std::string lastAddress;

};
