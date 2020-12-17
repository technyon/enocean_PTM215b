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

// #define BLE_DEBUG

class Enocean_PTM215b: public BLEAdvertisedDeviceCallbacks{
  public:
    Enocean_PTM215b();
    virtual ~Enocean_PTM215b();

    QueueHandle_t enocean_PTM215bBleIsrFlagQueue;

    virtual void initialize();

  private:
    TaskHandle_t TaskHandleEnocean_PTM215b;
    BaseType_t xHigherPriorityTaskWoken;
    void startEnocean_PTM215bBleXtask();
    void pushNotificationToQueue();
    void onResult(BLEAdvertisedDevice advertisedDevice) override;

    void handleButtonAction(uint8_t switchStatus);

    char securityKey[16] = {0};
    esp_bd_addr_t scannedBleAddress;

    /** Contents of a payload telegram */
    struct payload{
        char len[1] 			      = {0};
        char type[1] 			      = {0};
        char manufacturerId[2] 	= {0};
        char sequenceCounter[4] = {0};
        uint8_t switchStatus    = 0;
        char optionalData[4] 	  = {0};
    };

    /** Contents of a commissioning telegram */
    struct commissioning{
        char len[1] 				        = {0};
        char type[1] 				        = {0};
        char manufacturerId[2] 		  = {0};
        char sequenceCounter[4] 	  = {0};
        char securityKey[16] 		    = {0};
        char staticSourceAddress[6] = {0};
    };
};
