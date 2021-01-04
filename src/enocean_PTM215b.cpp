/*
 * enocean_PTM215b.cpp
 *
 *  Created on: 16 dec. 2020
 *      Author: Jeroen
 */

#include "enocean_PTM215b.h"
#include "mbedtls/aes.h"

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
      //Check if data (13 bytes) or commissioning (30) payload
      //TODO: make option to also read optional data when required, then data payload can be 13-17 bytes
      if(payloadLen == 13 ){
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
    #ifdef DEBUG_DATA
      log_d("BLE address: %s", bleAddress.c_str());
      printBuffer((byte*)dataPayloadBuffer.len, 1, false, "PayloadLen");
      printBuffer((byte*)dataPayloadBuffer.type, 1, false, "PayloadType");
      printBuffer((byte*)dataPayloadBuffer.manufacturerId, 2, false, "PayloadManufacturerId");
      log_d("PayloadseqCounter: %d", dataPayloadBuffer.sequenceCounter);
      log_d("PayloadSwitchStatus: %d", dataPayloadBuffer.switchStatus);
      // printBuffer((byte*)dataPayloadBuffer.optionalData, 4, false, "PayloadOptionalData");
      printBuffer((byte*)dataPayloadBuffer.receivedSecurityKey, 4, false, "PayloadsecurityKey");
    #endif

    
    //check if new sequence nr is larger than last (prevent replay or hack)
    if(bleSwitches[bleAddress].lastSequenceCounter < dataPayloadBuffer.sequenceCounter){
      securityKeyValid(bleAddress);
      handleSwitchAction(dataPayloadBuffer.switchStatus, bleAddress);
      //save last sequence nr
      bleSwitches[bleAddress].lastSequenceCounter = dataPayloadBuffer.sequenceCounter;
      //save last payload and address to be able to skip same message as is sent thrice every action
      lastNewDataPayload = dataPayloadBuffer;
      lastAddress = bleAddress;
    }
  }
}

bool Enocean_PTM215b::securityKeyValid(std::string bleAddress){
  unsigned char  nonce[13] = {0};
  unsigned char  leSourceAddress[6] = {0}; //little endian
  uint8_t a0Flag = 0x01;
  uint8_t b0Flag = 0x49;
  uint16_t inputLength = 9;
  unsigned char  a0[16] = {0};
  unsigned char  b0[16] = {0};
  unsigned char  b1[16] = {0};

  leSourceAddress[0] = strtol(bleAddress.substr(15,2).c_str(), NULL, 16);
  leSourceAddress[1] = strtol(bleAddress.substr(12,2).c_str(), NULL, 16);
  leSourceAddress[2] = strtol(bleAddress.substr(9,2).c_str(), NULL, 16);
  leSourceAddress[3] = strtol(bleAddress.substr(6,2).c_str(), NULL, 16);
  leSourceAddress[4] = strtol(bleAddress.substr(3,2).c_str(), NULL, 16);
  leSourceAddress[5] = strtol(bleAddress.substr(0,2).c_str(), NULL, 16);

  //construct nonce
  memcpy(&nonce[0], leSourceAddress, sizeof(leSourceAddress));
  nonce[6] = (dataPayloadBuffer.sequenceCounter >> (8*0)) & 0xff;
  nonce[7] = (dataPayloadBuffer.sequenceCounter >> (8*1)) & 0xff;
  nonce[8] = (dataPayloadBuffer.sequenceCounter >> (8*2)) & 0xff;
  nonce[9] = (dataPayloadBuffer.sequenceCounter >> (8*3)) & 0xff;
  nonce[10] = (0x00 >> (8*0)) & 0xff;
  nonce[11] = (0x00 >> (8*0)) & 0xff;
  nonce[12] = (0x00 >> (8*0)) & 0xff;

  //construct a0 input parameter
  a0[0] = (a0Flag >> (8*0)) & 0xff;
  memcpy(&a0[1], nonce, sizeof(nonce));
  a0[14] = (0x00 >> (8*0)) & 0xff;
  a0[15] = (0x00 >> (8*0)) & 0xff;

  //construct b0 input parameter
  b0[0] = (b0Flag >> (8*0)) & 0xff;
  memcpy(&b0[1], nonce, sizeof(nonce));
  b0[14] = (0x00 >> (8*0)) & 0xff;
  b0[15] = (0x00 >> (8*0)) & 0xff;

  //construct b1 input parameter
  b1[0] = (inputLength >> (8*1)) & 0xff;
  b1[1] = (inputLength >> (8*0)) & 0xff;
  memcpy(&b1[2], dataPayloadBuffer.len, sizeof(dataPayloadBuffer.len));
  memcpy(&b1[3], dataPayloadBuffer.type, sizeof(dataPayloadBuffer.type));
  memcpy(&b1[4], dataPayloadBuffer.manufacturerId, sizeof(dataPayloadBuffer.manufacturerId));
  b1[6] = (dataPayloadBuffer.sequenceCounter >> (8*0)) & 0xff;
  b1[7] = (dataPayloadBuffer.sequenceCounter >> (8*1)) & 0xff;
  b1[8] = (dataPayloadBuffer.sequenceCounter >> (8*2)) & 0xff;
  b1[9] = (dataPayloadBuffer.sequenceCounter >> (8*3)) & 0xff;
  b1[10] = (dataPayloadBuffer.switchStatus >> (8*0)) & 0xff;
  b1[11] = (0x00 >> (8*0)) & 0xff;
  b1[12] = (0x00 >> (8*0)) & 0xff;
  b1[13] = (0x00 >> (8*0)) & 0xff;
  b1[14] = (0x00 >> (8*0)) & 0xff;
  b1[15] = (0x00 >> (8*0)) & 0xff;

  mbedtls_aes_context aes;
  mbedtls_aes_init( &aes );

  char tempSecurityKey[16] = { 0x1C, 0xB9, 0x5C, 0xE6, 0x3F, 0x19, 0xAD, 0xC7, 0xE0, 0xFB, 0x92, 0xDA, 0x56, 0xD6, 0x92, 0x19};
  unsigned char x1[16] = {0};
  unsigned char x1a[16] = {0};
  unsigned char x2[16] = {0};
  unsigned char s0[16] = {0};
  unsigned char t0[16] = {0};

  printBuffer((byte*)tempSecurityKey, 16, false, "sec key");
  mbedtls_aes_setkey_enc( &aes, (const unsigned char*) tempSecurityKey, strlen(tempSecurityKey) * 8 );
  
  //calculate X1 from B0
  char testInput[16] = {0x49, 0xf9, 0x72, 0x01, 0x00, 0x15, 0xe2, 0x00, 0x00, 0x04, 0x2e, 0x00, 0x00, 0x00, 0x00, 0x00};
  printBuffer((byte*)testInput, 16, false, "test input");
  mbedtls_aes_crypt_ecb( &aes, MBEDTLS_AES_DECRYPT, (const unsigned char*)testInput, x1);
  printBuffer((byte*)x1, 16, false, "output x1");

  
  // //xor X1 B1
  // for(int i = 0; i < 16; ++i){
  //   x1a[i] = x1[i] ^ b1[i];
  // }

  // //calculate X2 from X1A
  // mbedtls_aes_crypt_ecb( &aes, MBEDTLS_AES_DECRYPT, (const unsigned char*)x1a, x2);
  
  // //calculate S0 from A0
  // mbedtls_aes_crypt_ecb( &aes, MBEDTLS_AES_DECRYPT, (const unsigned char*)a0, s0);

  // //xor X2 S0
  // for(int i = 0; i < 16; ++i){
  //   t0[i] = x2[i] ^ s0[i];
  // }

  // #ifdef DEBUG_ENCRYPTION
  //   printBuffer((byte*)nonce, sizeof(nonce), false, "Nonce:");
  //   printBuffer((byte*)a0, sizeof(a0), false, "A0   :");
  //   printBuffer((byte*)b0, sizeof(b0), false, "B0   :");
  //   printBuffer((byte*)b1, sizeof(b1), false, "B1   :");
  //   printBuffer((byte*)x1, sizeof(x1), false, "X1   :");
  //   printBuffer((byte*)x1a, sizeof(x1a), false, "X1A  :");
  //   printBuffer((byte*)x2, sizeof(x2), false, "X2   :");
  //   printBuffer((byte*)x2, sizeof(x2), false, "S2   :");
  //   printBuffer((byte*)t0, sizeof(t0), false, "T0   :");
  // #endif

  mbedtls_aes_free( &aes );
  return true;
}

void Enocean_PTM215b::handleCommissioningPayload(std::string bleAddress) {
  #ifdef DEBUG_DATA
    log_d("BLE address: %s", bleAddress.c_str());
    printBuffer((byte*)commissioningPayloadBuffer.len, 1, false, "PayloadLen");
    printBuffer((byte*)commissioningPayloadBuffer.type, 1, false, "PayloadType");
    printBuffer((byte*)commissioningPayloadBuffer.manufacturerId, 2, false, "PayloadManufacturerId");
    log_d("PayloadseqCounter: %d", commissioningPayloadBuffer.sequenceCounter);
    printBuffer((byte*)commissioningPayloadBuffer.securityKey, 16, false, "securitykey");
    printBuffer((byte*)commissioningPayloadBuffer.staticSourceAddress, 6, false, "PayloadOptionalData");
  #endif
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


