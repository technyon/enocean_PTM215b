/*
 * enocean_PTM215b.cpp
 *
 *  Created on: 16 dec. 2020
 *      Author: Jeroen
 */

#include "enocean_PTM215b.h"
#include "mbedtls/aes.h"
namespace PTM215b {

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
	log_d("TASK: PTM215b BLE scan task started");
	Enocean_PTM215b* enocean_PTM215bObj = static_cast<Enocean_PTM215b*>(pvParameters);
  BLEScan* pBLEScan = BLEDevice::getScan(); //get global scan-object
  pBLEScan->setAdvertisedDeviceCallbacks(enocean_PTM215bObj, true); //want duplicates as we will not be connecting and only listening to adv data
  pBLEScan->setActiveScan(true); //active scan uses more power, but get results faster
  pBLEScan->setInterval(17);
  pBLEScan->setWindow(23);

	while (1){
    if (enocean_PTM215bObj->registeredSwitchCount() > 0) {
      //scan for 10 seconds and then delete results to prevent memory leak if running for long time and registering different BLE devices
      pBLEScan->start(10,true);
      pBLEScan->clearResults();
    } else {
      log_w("No switches registered, waiting 5 sec...");
      delay(5000);
    }
  }
}

void repeatEventsTask(void * pvParameters) {
	log_d("TASK: PTM215b repeatEvents task started");
	Enocean_PTM215b* enocean_PTM215bObj = static_cast<Enocean_PTM215b*>(pvParameters);
  
	while (1){
    delay(REPEAT_INTERVAL);
    enocean_PTM215bObj->generateRepeatEvents();
  }
}

Enocean_PTM215b::Enocean_PTM215b(Eventhandler& handler, const boolean enableRepeatTask) : eventHandler(handler), enableRepeatTask(enableRepeatTask) {
}

Enocean_PTM215b::~Enocean_PTM215b(){
  if (repeatEventsTaskHandle) vTaskDelete(repeatEventsTaskHandle);
  if (bleScanTaskHandle) vTaskDelete(bleScanTaskHandle);
}

void Enocean_PTM215b::initialize() {
  log_d("Initializing PTM215b");
  if (!BLEDevice::getInitialized()) {
    log_e("BLEDevice not initialized");
    return;
  }
  startTasks();
}


void Enocean_PTM215b::startTasks(){
  xTaskCreatePinnedToCore(&bleScanTask, "PMT215_scanBleTask", 4096, this, 1, &bleScanTaskHandle, 1);
  if (enableRepeatTask) {
    xTaskCreatePinnedToCore(&repeatEventsTask, "PMT215_repeatEventsTask", 4096, this, 1, &repeatEventsTaskHandle, 1);
  }
}

void Enocean_PTM215b::onResult(BLEAdvertisedDevice advertisedDevice) {
  BLEAddress serverBleAddressObj {advertisedDevice.getAddress()};
  esp_bd_addr_t scannedBleAddress;
  memcpy(scannedBleAddress, serverBleAddressObj.getNative(), 6);

  uint8_t payloadLen = advertisedDevice.getPayloadLength();

  //check that the upper 2 byte of the Static Source Address are 0xE215. (Check that you get an ADV_NONCONN_IND PDU, not found option to retreive this info)
  if(scannedBleAddress[0] == PMT215B_STATIC_SOURCE_ADDRESS_FIRST_BYTE && scannedBleAddress[1] == PMT215B_STATIC_SOURCE_ADDRESS_SECOND_BYTE){
    //check that GAP AD Type is 0xFF and for correct manufacturer id
    if(advertisedDevice.getPayload()[1] == 0xFF && advertisedDevice.getPayload()[2] == 0xDA && advertisedDevice.getPayload()[3] == 0x03){
      //Check if data (13 bytes) or commissioning (30) payload
      //TODO: make option to also read optional data when required, then data payload can be 13-17 bytes
      if(payloadLen == 13 ){
        memcpy(&dataPayloadBuffer, advertisedDevice.getPayload(), payloadLen);
        handleDataPayload(serverBleAddressObj.toString());
      } else if(payloadLen == 30){
        memcpy(&commissioningPayloadBuffer, advertisedDevice.getPayload(), payloadLen+1);
        handleCommissioningPayload(serverBleAddressObj.toString());
      }
      else{
        log_e("BLE Payload size does not match for Enocean switch");
        return;
      }
    }
  }
}

void Enocean_PTM215b::handleDataPayload(std::string bleAddress) {
  if (switches.count(bleAddress) == 0) {
    log_w("Unknown address [%s]", bleAddress.c_str());
    return;
  }

  //ignore double/triple messages as switch always sends three messages per action
  if (switches[bleAddress].lastSequenceCounter < dataPayloadBuffer.sequenceCounter){
    #ifdef DEBUG_DATA
      log_d("## Stored data payload ##");
      log_d("BLE address: %s", bleAddress.c_str());
      log_d("PayloadLen: %d", dataPayloadBuffer.len);
      log_d("PayloadLen: %x", dataPayloadBuffer.type);
      printBuffer((byte*)dataPayloadBuffer.manufacturerId, 2, false, "PayloadManufacturerId");
      log_d("PayloadseqCounter: %d", dataPayloadBuffer.sequenceCounter);
      log_d("PayloadSwitchStatus: %d", dataPayloadBuffer.switchStatus);
      // printBuffer((byte*)dataPayloadBuffer.optionalData, 4, false, "PayloadOptionalData");
      printBuffer((byte*)dataPayloadBuffer.receivedSecurityKey, 4, false, "PayloadsecurityKey");
      log_d("## END data payload ##");
    #endif

    if (securityKeyValid(bleAddress)){
      //save last sequence nr
      switches[bleAddress].lastSequenceCounter = dataPayloadBuffer.sequenceCounter;
      handleSwitchAction(dataPayloadBuffer.switchStatus, bleAddress);
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

  //construct a0 input parameter
  a0[0] = a0Flag;
  memcpy(&a0[1], nonce, sizeof(nonce));

  //construct b0 input parameter
  b0[0] = b0Flag;
  memcpy(&b0[1], nonce, sizeof(nonce));

  //construct b1 input parameter
  b1[0] = (inputLength >> (8*1)) & 0xff;
  b1[1] = (inputLength >> (8*0)) & 0xff;
  b1[2] = dataPayloadBuffer.len;
  b1[3] = dataPayloadBuffer.type;
  memcpy(&b1[4], dataPayloadBuffer.manufacturerId, sizeof(dataPayloadBuffer.manufacturerId));
  b1[6] = (dataPayloadBuffer.sequenceCounter >> (8*0)) & 0xff;
  b1[7] = (dataPayloadBuffer.sequenceCounter >> (8*1)) & 0xff;
  b1[8] = (dataPayloadBuffer.sequenceCounter >> (8*2)) & 0xff;
  b1[9] = (dataPayloadBuffer.sequenceCounter >> (8*3)) & 0xff;
  b1[10] = dataPayloadBuffer.switchStatus;
  
  unsigned char x1[16] = {0};
  unsigned char x1a[16] = {0};
  unsigned char x2[16] = {0};
  unsigned char s0[16] = {0};
  unsigned char t0[16] = {0};

  mbedtls_aes_context aes;
  mbedtls_aes_init( &aes );

  mbedtls_aes_setkey_enc( &aes, (const unsigned char*) switches[bleAddress].securityKey, strlen(switches[bleAddress].securityKey) * 8 );
  
  //calculate X1 from B0
  mbedtls_aes_crypt_ecb( &aes, MBEDTLS_AES_ENCRYPT, (const unsigned char*)b0, x1);
  
  //xor X1 B1
  for(int i = 0; i < 16; ++i){
    x1a[i] = x1[i] ^ b1[i];
  }

  //calculate X2 from X1A
  mbedtls_aes_crypt_ecb( &aes, MBEDTLS_AES_ENCRYPT, (const unsigned char*)x1a, x2);
  
  //calculate S0 from A0
  mbedtls_aes_crypt_ecb( &aes, MBEDTLS_AES_ENCRYPT, (const unsigned char*)a0, s0);
  mbedtls_aes_free( &aes );

  //xor X2 S0
  for(int i = 0; i < 16; ++i){
    t0[i] = x2[i] ^ s0[i];
  }

  #ifdef DEBUG_ENCRYPTION
    log_d("## START encryption data ##");
    printBuffer((byte*)switches[bleAddress].securityKey, 16, false, "sec key");
    printBuffer((byte*)nonce, sizeof(nonce), false, "Nonce:");
    printBuffer((byte*)a0, sizeof(a0), false, "A0   :");
    printBuffer((byte*)b0, sizeof(b0), false, "B0   :");
    printBuffer((byte*)b1, sizeof(b1), false, "B1   :");
    printBuffer((byte*)x1, sizeof(x1), false, "X1   :");
    printBuffer((byte*)x1a, sizeof(x1a), false, "X1A  :");
    printBuffer((byte*)x2, sizeof(x2), false, "X2   :");
    printBuffer((byte*)s0, sizeof(s0), false, "S2   :");
    printBuffer((byte*)t0, sizeof(t0), false, "T0   :");
    log_d("## END encryption data ##");
  #endif

  if (memcmp(t0, dataPayloadBuffer.receivedSecurityKey, 4) == 0) {
    return true;
  }
  else{
    log_e("Incorrect security key");
    return false;
  }
}

void Enocean_PTM215b::handleCommissioningPayload(std::string bleAddress) {
  #ifdef DEBUG_COMMISSIONING_DATA
    log_d("## START commissioning payload ##");
    log_d("BLE address: %s", bleAddress.c_str());
    log_d("PayloadLen: %d", dataPayloadBuffer.len);
    log_d("PayloadLen: %x", dataPayloadBuffer.type);
    printBuffer((byte*)commissioningPayloadBuffer.manufacturerId, 2, false, "PayloadManufacturerId");
    log_d("PayloadseqCounter: %d", commissioningPayloadBuffer.sequenceCounter);
    printBuffer((byte*)commissioningPayloadBuffer.securityKey, 16, false, "securitykey");
    printBuffer((byte*)commissioningPayloadBuffer.staticSourceAddress, 6, false, "PayloadOptionalData");
    log_d("## END commissioning payload ##");
  #endif
}

std::string str_tolower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(), 
                   [](unsigned char c){ return std::tolower(c); }
                  );
    return s;
}

void Enocean_PTM215b::registerBleSwitch(const std::string bleAddress, const std::string securityKey, const uint8_t nodeIdA, const uint8_t nodeIdB){
  Switch bleSwitch;
  hexStringToCharArray(securityKey, bleSwitch.securityKey, 16);
  bleSwitch.nodeIdA0 = nodeIdA;
  bleSwitch.nodeIdA1 = nodeIdA;
  bleSwitch.nodeIdB0 = nodeIdB;
  bleSwitch.nodeIdB1 = nodeIdB;
  switches.insert(std::pair<std::string, Switch>(str_tolower(bleAddress), bleSwitch));

  #ifdef DEBUG_REGISTER_CONFIG
    log_d("## START Register BLE switch ##");
    log_d("BLE address: %s", str_tolower(bleAddress).c_str());
    printBuffer((byte*)bleSwitch.securityKey, strlen(bleSwitch.securityKey), false, "Security key");
    log_d("NodeA id: %d", bleSwitch.nodeIdA0);
    log_d("NodeB id: %d", bleSwitch.nodeIdB0);
    log_d("## END Register BLE switch ##");
  #endif
}

void Enocean_PTM215b::registerBleSwitch(const std::string bleAddress, const std::string securityKey, const uint8_t nodeIdA0, const uint8_t nodeIdA1, 
                                        const uint8_t nodeIdB0, const uint8_t nodeIdB1){
  Switch bleSwitch;
  hexStringToCharArray(securityKey, bleSwitch.securityKey, 16);
  bleSwitch.nodeIdA0 = nodeIdA0;
  bleSwitch.nodeIdA1 = nodeIdA1;
  bleSwitch.nodeIdB0 = nodeIdB0;
  bleSwitch.nodeIdB1 = nodeIdB1;
  switches.insert(std::pair<std::string, Switch>(str_tolower(bleAddress), bleSwitch));

  #ifdef DEBUG_REGISTER_CONFIG
    log_d("## START Register BLE switch ##");
    log_d("BLE address: %s", str_tolower(bleAddress).c_str());
    printBuffer((byte*)bleSwitch.securityKey, strlen(bleSwitch.securityKey), false, "Security key");
    log_d("NodeA0 id: %d", bleSwitch.nodeIdA0);
    log_d("NodeA1 id: %d", bleSwitch.nodeIdA1);
    log_d("NodeB0 id: %d", bleSwitch.nodeIdB0);
    log_d("NodeB1 id: %d", bleSwitch.nodeIdB1);
    log_d("## END Register BLE switch ##");
  #endif
}

void Enocean_PTM215b::handleSwitchAction(const uint8_t switchStatus, const std::string bleAddress){
  log_v("handling button action: %d from %s", switchStatus, bleAddress.c_str());
  if (switches.count(bleAddress) == 0) {
    log_w("No switch registered for address [%d]", bleAddress.c_str());
    return;
  }

  Switch bleSwitch = switches[bleAddress];

  // TODO Allow for simultaneous events on A and B

  uint8_t nodeId;
  if (switchStatus & 0b00000010) {
     nodeId = bleSwitch.nodeIdA0;
  } else if (switchStatus & 0b00000100) {
    nodeId = bleSwitch.nodeIdA1;
  } else if (switchStatus & 0b00001000) {
    nodeId = bleSwitch.nodeIdB0;
  } else if (switchStatus & 0b00010000) {
    nodeId = bleSwitch.nodeIdB1;
  } else {
    log_e("Invalid switchStatus [0x%02X]", switchStatus);
    return;
  }

  // TODO Allow for simultaneous Directions, possible with a 4-button PTM
  Direction direction;
  if (switchStatus & 0b00001010) {
     direction = Direction::Up;
  } else if (switchStatus & 0b00010100) {
    direction = Direction::Down;
  } else {
    log_e("No valid Direction in switchStatus [0x%02X]", switchStatus);
    return;
  }

  ActionType type = (ActionType)(switchStatus & 0x01);

  SwitchAction action;
  action.nodeId = nodeId;
  action.actionType = type;
  action.direction = direction;

  SwitchEvent event;
  event.nodeId = nodeId;
  event.direction = direction;
  
  if (type == ActionType::Release) {
    if (millis() - INITIAL_REPEAT_WAIT < actions[nodeId].pushStartTime ){
      event.eventType = EventType::ReleaseShort;
    } else {
      event.eventType = EventType::ReleaseLong;
    }
    eventHandler.handleEvent(event);
    actions.erase(nodeId);
  } else {
    action.pushStartTime = millis();
    actions[nodeId] = action;
    event.eventType = EventType::Pushed;
    eventHandler.handleEvent(event);
  }
}

void Enocean_PTM215b::generateRepeatEvents() {
  for (auto const& pair : actions) {
    SwitchAction action = pair.second;
    if (millis() - INITIAL_REPEAT_WAIT > action.pushStartTime ) {
      SwitchEvent event;
      event.nodeId = action.nodeId;
      event.direction = action.direction;
      event.eventType = EventType::Repeat;
      eventHandler.handleEvent(event);
    }
  }
}

void Enocean_PTM215b::hexStringToCharArray(std::string stringInput, char * charArrayOutput, uint8_t byteLength) {
  for(int i = 0; i < byteLength; i++){
    charArrayOutput[i] = strtol(stringInput.substr(i*2,2).c_str(), NULL, 16);
  }
}

} // namespace