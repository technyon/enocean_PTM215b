/*
 * enocean_PTM215b.cpp
 *
 *  Created on: 16 dec. 2020
 *      Author: Jeroen
 */

#include "enocean_PTM215b.h"

void printBuffer(const byte* buff, const uint8_t size, const boolean asChars, const char* header) {
  delay(100); //delay otherwise first part of print will not be shown 
  char tmp[4];
  
  if (strlen(header) > 0) {
    Serial.print(header);
    Serial.print(": ");
  }
  for (int i = 0; i < size; i++) {
    if (asChars) {
      Serial.print((char)buff[i]);
    } else {
      sprintf(tmp, "%02x", buff[i]);
      Serial.print(tmp);
      Serial.print(" ");
    }
  }
  Serial.println();
}

void enocean_PTM215bBleTask(void * pvParameters) {
	log_d("TASK: enocean_PTM215b BLE task started");
	Enocean_PTM215b* enocean_PTM215bObj = static_cast<Enocean_PTM215b*>(pvParameters);

	BLEScan* pBLEScan;
  BLEScanResults foundDevices;
  pBLEScan = BLEDevice::getScan(); //create new scan
  pBLEScan->setAdvertisedDeviceCallbacks(enocean_PTM215bObj);
  pBLEScan->setActiveScan(true); //active scan uses more power, but get results faster
  
	while (1){
    pBLEScan->start(3); //Scan for 3 seconds
    delay(100);
  }
}

Enocean_PTM215b::Enocean_PTM215b(){}

Enocean_PTM215b::~Enocean_PTM215b(){}

void Enocean_PTM215b::initialize() {
  log_d("Initializing PTM215b");

  BLEDevice::init("ESP32_client");
  startEnocean_PTM215bBleXtask();
}

void Enocean_PTM215b::startEnocean_PTM215bBleXtask(){
  enocean_PTM215bBleIsrFlagQueue=xQueueCreate(10,sizeof(uint8_t));
  TaskHandleEnocean_PTM215b = NULL;
  xTaskCreatePinnedToCore(&enocean_PTM215bBleTask, "enocean_PTM215bBleTask", 4096, this, 1, &TaskHandleEnocean_PTM215b, 1);
}

void Enocean_PTM215b::onResult(BLEAdvertisedDevice advertisedDevice) {
  esp_bd_addr_t scannedBleAddress;
  static BLEAddress *serverBleAddressObj;

  serverBleAddressObj = new BLEAddress(advertisedDevice.getAddress());
  memcpy(scannedBleAddress, serverBleAddressObj->getNative(), 6);

  payload payloadBuffer;
  memcpy(&payloadBuffer, advertisedDevice.getPayload(), advertisedDevice.getPayloadLength()+1);
  
  
  //Check that you get an ADV_NONCONN_IND PDU, check that GAP AD Type is 0xFF, 
  //check that Manufacturer ID is 0x03DA and check that the upper 2 byte of the Static Source Address are 0xE215.
  if(scannedBleAddress[0] == PMT215B_STATIC_SOURCE_ADDRESS_FIRST_BYTE && scannedBleAddress[1] == PMT215B_STATIC_SOURCE_ADDRESS_SECOND_BYTE && payloadBuffer.type[0] == 0xff){
    log_d("Found enocean PTM215b: %s", advertisedDevice.toString().c_str());
    
    char* pHex = BLEUtils::buildHexData(nullptr, (uint8_t*)advertisedDevice.getManufacturerData().data(), advertisedDevice.getManufacturerData().length());
    log_d("manufacturer data: %s", pHex);
    free(pHex);
    

    log_d("Address data: %x:%x:%x:%x:%x:%x", scannedBleAddress[0], scannedBleAddress[1], 
          scannedBleAddress[2], scannedBleAddress[3], scannedBleAddress[4], scannedBleAddress[5]);
    
    pHex = BLEUtils::buildHexData(nullptr, advertisedDevice.getPayload(), advertisedDevice.getPayloadLength());
    log_d("payload data: %s", pHex);
    free(pHex);

    
    // log_d("payload: %02x %02x %04x %08x %02x %08x", payloadBuffer.len[0], payloadBuffer.type[0], payloadBuffer.manufacturerId[0], payloadBuffer.sequenceCounter[0], payloadBuffer.switchStatus[0], payloadBuffer.optionalData[0]);
    printBuffer((byte*)payloadBuffer.len, 1, false, "PayloadLen:");
    printBuffer((byte*)payloadBuffer.type, 1, false, "PayloadType:");
    printBuffer((byte*)payloadBuffer.manufacturerId, 2, false, "PayloadManufacturerId:");
    printBuffer((byte*)payloadBuffer.sequenceCounter, 4, false, "PayloadseqCounter:");
    printBuffer((byte*)payloadBuffer.switchStatus, 1, false, "PayloadSwitchStatus:");
    printBuffer((byte*)payloadBuffer.optionalData, 4, false, "PayloadOptionalData:");


    
  }
}


