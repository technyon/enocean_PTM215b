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

class Enocean_PTM215b{
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
    void onScanResult(BLEAdvertisedDevice advertisedDevice);
};

class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks 
{
    void onResult(BLEAdvertisedDevice advertisedDevice) {
      String Scaned_BLE_Address;
      static BLEAddress *Server_BLE_Address;
      
      Serial.printf("Scan Result: %s \n", advertisedDevice.toString().c_str());
      Server_BLE_Address = new BLEAddress(advertisedDevice.getAddress());
      
      Scaned_BLE_Address = Server_BLE_Address->toString().c_str(); 
    }
};