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

void bleScanTask(void * pvParameters) {
	log_d("TASK: BLE scan task started");
	Enocean_PTM215b* enocean_PTM215bObj = static_cast<Enocean_PTM215b*>(pvParameters);
  BLEScan* pBLEScan;
  BLEScanResults foundDevices;
  pBLEScan = BLEDevice::getScan(); //create new scan
  pBLEScan->setAdvertisedDeviceCallbacks(enocean_PTM215bObj, true); //want duplicates as we will not be connecting and only listening to adv data
  pBLEScan->setActiveScan(true); //active scan uses more power, but get results faster
  pBLEScan->setInterval(17);
  pBLEScan->setWindow(23);

	while (1){
    //scan for 10 seconds and then delete results to prevent memory leak if running for long time and registering different BLE devices
    pBLEScan->start(10,true);
    pBLEScan->clearResults();
  }
}

void enocean_PTM215bBleTask(void * pvParameters) {
	log_d("TASK: enocean_PTM215b BLE task started");
	Enocean_PTM215b* enocean_PTM215bObj = static_cast<Enocean_PTM215b*>(pvParameters);
  
	while (1){
    delay(500);
    //check for long button press
    for (auto bleSwitch : enocean_PTM215bObj->bleSwitches){
      if(bleSwitch.second.rockerA0Pushed && ( (millis() - LONG_PRESS_INTERVAL_MS) > bleSwitch.second.rockerA0PushedStartTime) ){
        bleSwitch.second.rockerA0PushedType = PUSHED_LONG;
        log_d("A0 pushed long");
        //TODO: notify observers
      }else if(bleSwitch.second.rockerA1Pushed && ( (millis() - LONG_PRESS_INTERVAL_MS) > bleSwitch.second.rockerA1PushedStartTime) ){
        bleSwitch.second.rockerA1PushedType = PUSHED_LONG;
        log_d("A1 pushed long");
        //TODO: notify observers
      }else if(bleSwitch.second.rockerB0Pushed && ( (millis() - LONG_PRESS_INTERVAL_MS) > bleSwitch.second.rockerB0PushedStartTime) ){
        bleSwitch.second.rockerB0PushedType = PUSHED_LONG;
        log_d("B0 pushed long");
        //TODO: notify observers
      }else if(bleSwitch.second.rockerB1Pushed && ( (millis() - LONG_PRESS_INTERVAL_MS) > bleSwitch.second.rockerB1PushedStartTime) ){
        bleSwitch.second.rockerB1PushedType = PUSHED_LONG;
        log_d("B1 pushed long");
        //TODO: notify observers
      }
    }
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
  xTaskCreatePinnedToCore(&bleScanTask, "scanBleTask", 4096, this, 1, &TaskHandleBleScan, 1);
}

void Enocean_PTM215b::onResult(BLEAdvertisedDevice advertisedDevice) {
  static BLEAddress *serverBleAddressObj;
  uint8_t payloadLen = 0;
  esp_bd_addr_t scannedBleAddress;
  serverBleAddressObj = new BLEAddress(advertisedDevice.getAddress());
  memcpy(scannedBleAddress, serverBleAddressObj->getNative(), 6);

  payloadLen = advertisedDevice.getPayloadLength();

  //check that the upper 2 byte of the Static Source Address are 0xE215. (Check that you get an ADV_NONCONN_IND PDU, not found option to retreive this info)
  if(scannedBleAddress[0] == PMT215B_STATIC_SOURCE_ADDRESS_FIRST_BYTE && scannedBleAddress[1] == PMT215B_STATIC_SOURCE_ADDRESS_SECOND_BYTE){
    //check that GAP AD Type is 0xFF and for correct manufacturer id
    if(advertisedDevice.getPayload()[1] == 0xFF && advertisedDevice.getPayload()[2] == 0xDA && advertisedDevice.getPayload()[3] == 0x03){
      //Check if data (13-17 bytes) or commissioning (30) payload
      if(payloadLen < 17 ){
        memcpy(&dataPayloadBuffer, advertisedDevice.getPayload(), payloadLen+1);
        handleDataPayload(serverBleAddressObj->toString());
      } else if(payloadLen == 30){
        memcpy(&commissioningPayloadBuffer, advertisedDevice.getPayload(), payloadLen+1);
        handleCommissioningPayload(serverBleAddressObj->toString());
      }
      else{
        log_e("BLE Payload size does not match for Enocean switch");
        return;
      }
    }
  }
  delete serverBleAddressObj;
}

void Enocean_PTM215b::handleDataPayload(std::string bleAddress) {
  //TODO: needs to move to routine that handles registering/storing ble switches 
  registerBleSwitch(bleAddress, 0);

  //ignore double/triple messages as switch always sends three messages per action
  if(dataPayloadBuffer.switchStatus != lastNewDataPayload.switchStatus || bleAddress != lastAddress ){
    // log_d("BLE address: %s", bleAddress.c_str());
    // printBuffer((byte*)dataPayloadBuffer.len, 1, false, "PayloadLen");
    // printBuffer((byte*)dataPayloadBuffer.type, 1, false, "PayloadType");
    // printBuffer((byte*)dataPayloadBuffer.manufacturerId, 2, false, "PayloadManufacturerId");
    // log_d("PayloadseqCounter: %d", dataPayloadBuffer.sequenceCounter);
    // log_d("PayloadSwitchStatus: %d", dataPayloadBuffer.switchStatus);
    // printBuffer((byte*)dataPayloadBuffer.optionalData, 4, false, "PayloadOptionalData");
    printBuffer((byte*)dataPayloadBuffer.securityKey, 4, false, "PayloadsecurityKey");

    
    //check if new sequence nr is larger than last (prevent replay or hack)
    if(bleSwitches[bleAddress].lastSequenceCounter < dataPayloadBuffer.sequenceCounter){
      handleSwitchAction(dataPayloadBuffer.switchStatus, bleAddress);
      //save last sequence nr
      bleSwitches[bleAddress].lastSequenceCounter = dataPayloadBuffer.sequenceCounter;
      //save last payload and address to be able to skip same message as is sent thrice every action
      lastNewDataPayload = dataPayloadBuffer;
      lastAddress = bleAddress;
    }
  }
}

void Enocean_PTM215b::handleCommissioningPayload(std::string bleAddress) {
  //  log_d("BLE address: %s", bleAddress.c_str());
    printBuffer((byte*)commissioningPayloadBuffer.len, 1, false, "PayloadLen");
    printBuffer((byte*)commissioningPayloadBuffer.type, 1, false, "PayloadType");
    printBuffer((byte*)commissioningPayloadBuffer.manufacturerId, 2, false, "PayloadManufacturerId");
    log_d("PayloadseqCounter: %d", commissioningPayloadBuffer.sequenceCounter);
    printBuffer((byte*)commissioningPayloadBuffer.securityKey, 16, false, "securitykey");
    printBuffer((byte*)commissioningPayloadBuffer.staticSourceAddress, 6, false, "PayloadOptionalData");
}

void Enocean_PTM215b::registerBleSwitch(std::string bleAddress, uint8_t switchId){
  bleSwitch buffer;
  bleSwitches.insert(std::pair<std::string,bleSwitch>(bleAddress,buffer) );
  bleSwitches[bleAddress].switchId = switchId;
  bleSwitches[bleAddress].rockerType = ROCKER_A;
}

void Enocean_PTM215b::handleSwitchResult(std::string bleAddress, uint8_t rocker, uint8_t switchResult){
  // code to handle switch result
}

void Enocean_PTM215b::handleSwitchAction(uint8_t switchStatus, std::string bleAddress){
  log_v("handling button action: %d from %s", switchStatus, bleAddress.c_str());

  switch (switchStatus)
  {
    case BT_ENOCEAN_SWITCH_A0_PUSH:{
      // log_d("BT_ENOCEAN_SWITCH_A0_PUSH");
      bleSwitches[bleAddress].rockerA0Pushed = true;
      bleSwitches[bleAddress].rockerA0PushedStartTime = millis();
      break;
    }
    case BT_ENOCEAN_SWITCH_A0_RELEASE:{
      bleSwitches[bleAddress].rockerA0Pushed = false;
      if(millis() - LONG_PRESS_INTERVAL_MS < bleSwitches[bleAddress].rockerA0PushedStartTime  ){
        bleSwitches[bleAddress].rockerA0PushedType = PUSHED_SHORT;
        bleSwitches[bleAddress].rockerANotificationPending = true;
        log_d("A0 pushed short");
        
      }
      break;
    }
    case BT_ENOCEAN_SWITCH_A1_PUSH:{
      bleSwitches[bleAddress].rockerA1Pushed = true;
      bleSwitches[bleAddress].rockerA1PushedStartTime = millis();
      break;
    }
    case BT_ENOCEAN_SWITCH_A1_RELEASE:{
      bleSwitches[bleAddress].rockerA1Pushed = false;
      if(millis() - LONG_PRESS_INTERVAL_MS < bleSwitches[bleAddress].rockerA1PushedStartTime  ){
        bleSwitches[bleAddress].rockerA1PushedType = PUSHED_SHORT;
        bleSwitches[bleAddress].rockerANotificationPending = true;
        log_d("A1 pushed short");
        //TODO: notifyobserver
      }
      break;
    }
    case BT_ENOCEAN_SWITCH_B0_PUSH:{
      bleSwitches[bleAddress].rockerB0Pushed = true;
      bleSwitches[bleAddress].rockerB0PushedStartTime = millis();
      break;
    }
    case BT_ENOCEAN_SWITCH_B0_RELEASE:{
      bleSwitches[bleAddress].rockerB0Pushed = false;
      if(millis() - LONG_PRESS_INTERVAL_MS < bleSwitches[bleAddress].rockerB0PushedStartTime  ){
        bleSwitches[bleAddress].rockerB0PushedType = PUSHED_SHORT;
        bleSwitches[bleAddress].rockerBNotificationPending = true;
        log_d("B0 pushed short");
        //TODO: notifyobserver
      }
      break;
    }
    case BT_ENOCEAN_SWITCH_B1_PUSH:{
      bleSwitches[bleAddress].rockerB1Pushed = true;
      bleSwitches[bleAddress].rockerB1PushedStartTime = millis();
      break;
    }
    case BT_ENOCEAN_SWITCH_B1_RELEASE:{
      bleSwitches[bleAddress].rockerB1Pushed = false;
      if(millis() - LONG_PRESS_INTERVAL_MS < bleSwitches[bleAddress].rockerB1PushedStartTime  ){
        bleSwitches[bleAddress].rockerB1PushedType = PUSHED_SHORT;
        bleSwitches[bleAddress].rockerBNotificationPending = true;
        bleSwitches[bleAddress].rockerB1Pushed = false;
        log_d("B1 pushed short");
        //TODO: notifyobserver
      }
      break;
    }
    case BT_ENOCEAN_SWITCH_PUSH:{
      log_e("BLE switch %s pushed but undefined.", bleAddress.c_str());
      break;
    }
    case BT_ENOCEAN_SWITCH_RELEASE:{
      log_e("BLE switch %s released but undefined.", bleAddress.c_str());
      break;
    }
    default:{
      log_e("Unknown button action");
      break;
    }
  }
}


