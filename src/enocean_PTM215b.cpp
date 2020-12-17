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
    pBLEScan->start(1); //Scan for 3 seconds
    delay(1);
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
  static BLEAddress *serverBleAddressObj;
  payload payloadBuffer;
  uint8_t payloadLen = 0;

  serverBleAddressObj = new BLEAddress(advertisedDevice.getAddress());
  memcpy(scannedBleAddress, serverBleAddressObj->getNative(), 6);

  payloadLen = advertisedDevice.getPayloadLength();

  //check that the upper 2 byte of the Static Source Address are 0xE215. (Check that you get an ADV_NONCONN_IND PDU, not found option to retreive this info)
  if(scannedBleAddress[0] == PMT215B_STATIC_SOURCE_ADDRESS_FIRST_BYTE && scannedBleAddress[1] == PMT215B_STATIC_SOURCE_ADDRESS_SECOND_BYTE){
    if(payloadLen < 14){
      memcpy(&payloadBuffer, advertisedDevice.getPayload(), payloadLen+1);
    }
    else{
      log_e("BLE Payload too large");
      return;
    }
    // check that GAP AD Type is 0xFF, check that Manufacturer ID is 0x03DA
    if(payloadBuffer.type[0] == 0xff && payloadBuffer.manufacturerId[0] == 0xDA && payloadBuffer.manufacturerId[1] == 0x03){
      // printBuffer((byte*)scannedBleAddress, 6, false, "BLE address");
      // printBuffer((byte*)payloadBuffer.len, 1, false, "PayloadLen");
      // printBuffer((byte*)payloadBuffer.type, 1, false, "PayloadType");
      // printBuffer((byte*)payloadBuffer.manufacturerId, 2, false, "PayloadManufacturerId");
      // printBuffer((byte*)payloadBuffer.sequenceCounter, 4, false, "PayloadseqCounter");
      // log_d("PayloadSwitchStatus: %d", payloadBuffer.switchStatus);
      // printBuffer((byte*)payloadBuffer.optionalData, 4, false, "PayloadOptionalData");

      handleButtonAction(payloadBuffer.switchStatus);
    }
  }
}

void Enocean_PTM215b::handleButtonAction(uint8_t switchStatus){
  log_d("handling button action: %d", switchStatus);
  switch (switchStatus)
  {
  case BT_ENOCEAN_SWITCH_OA_PUSH:
    log_d("BT_ENOCEAN_SWITCH_OA_PUSH");
    /* code */
    break;
  case BT_ENOCEAN_SWITCH_OA_RELEASE:
    log_d("BT_ENOCEAN_SWITCH_OA_PUSH");
    /* code */
    break;
  case BT_ENOCEAN_SWITCH_IA_PUSH:
    log_d("BT_ENOCEAN_SWITCH_OA_PUSH");
    /* code */
    break;
  case BT_ENOCEAN_SWITCH_IA_RELEASE:
    log_d("BT_ENOCEAN_SWITCH_OA_PUSH");
    /* code */
    break;
  case BT_ENOCEAN_SWITCH_OB_PUSH:
    log_d("BT_ENOCEAN_SWITCH_OA_PUSH");
    /* code */
    break;
  case BT_ENOCEAN_SWITCH_OB_RELEASE:
    log_d("BT_ENOCEAN_SWITCH_OA_PUSH");
    /* code */
    break;
  case BT_ENOCEAN_SWITCH_IB_PUSH:
    log_d("BT_ENOCEAN_SWITCH_OA_PUSH");
    /* code */
    break;
  case BT_ENOCEAN_SWITCH_IB_RELEASE:
    log_d("BT_ENOCEAN_SWITCH_OA_PUSH");
    /* code */
    break;
  case BT_ENOCEAN_SWITCH_PUSH:
    log_d("BT_ENOCEAN_SWITCH_PUSH");
    /* code */
    break;
  case BT_ENOCEAN_SWITCH_RELEASE:
    log_d("BT_ENOCEAN_SWITCH_RELEASE");
    /* code */
    break;
  
  default:
    log_e("Unknown button action");
    break;
  }
}


