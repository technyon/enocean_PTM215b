/*
 * enocean_PTM215b.cpp
 *
 *  Created on: 16 dec. 2020
 *      Author: Jeroen
 */

#include "enocean_PTM215b.h"
#include "enocean_utils.h"
#include "esp_task_wdt.h"
#include "mbedtls/aes.h"
#include <algorithm>

// #define CONFIG_BT_NIMBLE_PINNED_TO_CORE 1

namespace PTM215b {

void bleScanTask(void* pvParameters) {
#ifdef DEBUG_PTM215
  log_d("TASK: PTM215b BLE scan task started on core: %d", xPortGetCoreID());
#endif
  //TODO implement watchdog? Is triggered by the blocking start call below
  Enocean_PTM215b* enocean_PTM215bObj = static_cast<Enocean_PTM215b*>(pvParameters);
  NimBLEScan* pBLEScan                = NimBLEDevice::getScan();    // get global scan-object
  pBLEScan->setAdvertisedDeviceCallbacks(enocean_PTM215bObj, true); // want duplicates as we will not be connecting and only listening to adv data
  pBLEScan->setActiveScan(true);                                    // active scan uses more power, but get results faster
  pBLEScan->setInterval(17);
  pBLEScan->setWindow(17); // Must be between 4 and 16K and smaller or equal to Interval
  esp_task_wdt_add(NULL);
  while (1) {
    esp_task_wdt_reset();
    // if (enocean_PTM215bObj->registeredSwitchCount() > 0) {
      // scan for 1 seconds and then delete results to prevent memory leak if
      // running for long time and registering different BLE devices
      pBLEScan->start(1, true);
      pBLEScan->clearResults();
    // } else {
    //   log_w("No switches registered, waiting 3 sec...");
    //   delay(3000);
    // }
    // delay(10);
  }
}

void repeatEventsTask(void* pvParameters) {
#ifdef DEBUG_PTM215
  log_d("TASK: PTM215b repeatEvents task started on core: %d", xPortGetCoreID());
#endif
  esp_task_wdt_add(NULL);
  Enocean_PTM215b* enocean_PTM215bObj = static_cast<Enocean_PTM215b*>(pvParameters);

  while (1) {
    esp_task_wdt_reset();
    delay(REPEAT_INTERVAL);
    enocean_PTM215bObj->generateRepeatEvents();
  }
}

Enocean_PTM215b::Enocean_PTM215b(const boolean enableRepeatTask)
    : enableRepeatTask(enableRepeatTask) {}

Enocean_PTM215b::~Enocean_PTM215b() {
  if (repeatEventsTaskHandle)
    vTaskDelete(repeatEventsTaskHandle);
  if (bleScanTaskHandle)
    vTaskDelete(bleScanTaskHandle);
}

void Enocean_PTM215b::initialize() {
#ifdef DEBUG_PTM215
  log_d("Initializing PTM215b");
#endif
  if (!NimBLEDevice::getInitialized()) {
    log_e("BLEDevice not initialized");
    return;
  }
  startTasks();
}

void Enocean_PTM215b::startTasks() {
  xTaskCreatePinnedToCore(&bleScanTask, "PMT215_scanBleTask", 4096, this, 1, &bleScanTaskHandle, CONFIG_BT_NIMBLE_PINNED_TO_CORE);
  if (enableRepeatTask) {
    xTaskCreatePinnedToCore(&repeatEventsTask, "PMT215_repeatEventsTask", 4096, this, 1, &repeatEventsTaskHandle, CONFIG_BT_NIMBLE_PINNED_TO_CORE);
    //default suspended
    suspendRepeatTask(true);
  }
}

void Enocean_PTM215b::suspendRepeatTask(bool suspend) {
  if (repeatEventsTaskHandle != NULL) {
    if (suspend) {
      esp_task_wdt_delete(repeatEventsTaskHandle);
      vTaskSuspend(repeatEventsTaskHandle);
#ifdef DEBUG_PTM215
      log_d("Repeat task suspended");
#endif
    } else if (isSuspended(repeatEventsTaskHandle)) {
      vTaskResume(repeatEventsTaskHandle);
      esp_task_wdt_add(repeatEventsTaskHandle);
#ifdef DEBUG_PTM215
      log_d("Repeat task resumed");
#endif
    }
  }
}

bool Enocean_PTM215b::isSuspended(TaskHandle_t taskHandle) {
  if (taskHandle) {
    eTaskState state = eTaskGetState(taskHandle);
    return ((state == eSuspended) || (state == eDeleted));
  }
  return false;
}

void Enocean_PTM215b::setScanTaskPriority(uint8_t prio) {
  vTaskPrioritySet(bleScanTaskHandle, prio);
}

void Enocean_PTM215b::setRepeatTaskPriority(uint8_t prio) {
  vTaskPrioritySet(repeatEventsTaskHandle, prio);
}

void Enocean_PTM215b::onResult(NimBLEAdvertisedDevice* advertisedDevice) {

  NimBLEAddress bleAddress = advertisedDevice->getAddress();
  // check that the upper 2 byte of the Static Source Address are 0xE215.
  // Note NimBLEs getNative() returns byte in reverse order
  if (memcmp(bleAddress.getNative() + 4, PMT215B_STATIC_SOURCE_ADDRESS, sizeof(PMT215B_STATIC_SOURCE_ADDRESS)) == 0) {

    Payload payload = getPayload(advertisedDevice);

    // check that GAP AD Type is 0xFF and for correct manufacturer id
    if ((payload.type == PMT215B_PAYLOAD_TYPE) && (memcmp(payload.manufacturerId, PMT215B_PAYLOAD_MANUFACTURER, sizeof(PMT215B_PAYLOAD_MANUFACTURER)) == 0)) {
      if (payload.payloadType == Data) {
        handleDataPayload(bleAddress, payload);
      } else {
        handleCommissioningPayload(bleAddress, payload);
      }
    }
  }
}

Enocean_PTM215b::Payload Enocean_PTM215b::getPayload(NimBLEAdvertisedDevice* advertisedDevice) {
  Payload payload;

  // Pointer to first byte of payload to read next
  uint8_t* nextPayload = advertisedDevice->getPayload();

  printBuf(advertisedDevice->getPayload(), advertisedDevice->getPayloadLength(), false, "BLE Message");
  memcpy(&payload, advertisedDevice->getPayload(), 8);
  nextPayload = nextPayload + 8;

  if (payload.len <= 16) {
    // Data payload
    payload.payloadType       = Data;
    payload.data.switchStatus = *nextPayload++;
    if (payload.len > 12) {
      memcpy(&payload.data.optionalData, nextPayload, payload.len - 12);
      nextPayload = nextPayload + (payload.len - 12);
    }
    memcpy(&payload.data.receivedSecurityKey, nextPayload, 4);

  } else if (payload.len == 29) {
    // commissioning payload
    payload.payloadType = Commisioning;
    memcpy(&payload.commissioning, nextPayload, 22);
  }

  return payload;
}

void Enocean_PTM215b::handleDataPayload(NimBLEAddress& bleAddress, Payload& payload) {
  if (switches.count(bleAddress) == 0) {
    log_w("Unknown address [%s]", bleAddress.toString().c_str());
    return;
  }

  // ignore double/triple messages as switch always sends three messages per
  // action
  if (switches[bleAddress].lastSequenceCounter < payload.sequenceCounter) {
#ifdef DEBUG_PTM215
#ifdef DEBUG_DATA
    log_d("## Stored data payload ##");
    log_d("BLE address: %s", bleAddress.toString().c_str());
    printBuf((byte*)payload.manufacturerId, 2, false, "PayloadManufacturerId");
    log_d("PayloadseqCounter: %d", payload.sequenceCounter);
    log_d("PayloadSwitchStatus: %d", payload.switchStatus);
    printBuf((byte*)payload.receivedSecurityKey, 4, false,
             "PayloadsecurityKey");
    log_d("## END data payload ##");
#endif
#endif

    if (securityKeyValid(bleAddress, payload)) {
      // save last sequence nr
      switches[bleAddress].lastSequenceCounter = payload.sequenceCounter;
      handleSwitchAction(payload.data.switchStatus, bleAddress);
    }
  }
}

bool Enocean_PTM215b::securityKeyValid(NimBLEAddress& bleAddress, Payload& payload) {
  unsigned char nonce[13] = {0};
  uint8_t a0Flag          = 0x01;
  uint8_t b0Flag          = 0x49;
  uint16_t inputLength    = 9;
  unsigned char a0[16]    = {0};
  unsigned char b0[16]    = {0};
  unsigned char b1[16]    = {0};

  std::string bleAddressString = bleAddress.toString();

  nonce[0] = strtol(bleAddressString.substr(15, 2).c_str(), NULL, 16);
  nonce[1] = strtol(bleAddressString.substr(12, 2).c_str(), NULL, 16);
  nonce[2] = strtol(bleAddressString.substr(9, 2).c_str(), NULL, 16);
  nonce[3] = strtol(bleAddressString.substr(6, 2).c_str(), NULL, 16);
  nonce[4] = strtol(bleAddressString.substr(3, 2).c_str(), NULL, 16);
  nonce[5] = strtol(bleAddressString.substr(0, 2).c_str(), NULL, 16);

  // construct nonce
  memcpy(&nonce[6], &payload.sequenceCounter, sizeof(payload.sequenceCounter));

  // construct a0 input parameter
  a0[0] = a0Flag;
  memcpy(&a0[1], nonce, sizeof(nonce));

  // construct b0 input parameter
  b0[0] = b0Flag;
  memcpy(&b0[1], nonce, sizeof(nonce));

  // construct b1 input parameter
  b1[0] = (inputLength >> (8 * 1)) & 0xff;
  b1[1] = (inputLength >> (8 * 0)) & 0xff;
  b1[2] = payload.len;
  b1[3] = payload.type;
  memcpy(&b1[4], payload.manufacturerId, sizeof(payload.manufacturerId));
  memcpy(&b1[6], &payload.sequenceCounter, sizeof(payload.sequenceCounter));
  b1[10] = payload.data.switchStatus;

  unsigned char x1[16]  = {0};
  unsigned char x1a[16] = {0};
  unsigned char x2[16]  = {0};
  unsigned char s0[16]  = {0};
  unsigned char t0[16]  = {0};

  mbedtls_aes_context aes;
  mbedtls_aes_init(&aes);

  mbedtls_aes_setkey_enc(
      &aes, (const unsigned char*)switches[bleAddress].securityKey, sizeof(switches[bleAddress].securityKey) * 8);

  // calculate X1 from B0
  mbedtls_aes_crypt_ecb(&aes, MBEDTLS_AES_ENCRYPT, (const unsigned char*)b0,
                        x1);

  // xor X1 B1
  for (int i = 0; i < 16; ++i) {
    x1a[i] = x1[i] ^ b1[i];
  }

  // calculate X2 from X1A
  mbedtls_aes_crypt_ecb(&aes, MBEDTLS_AES_ENCRYPT, (const unsigned char*)x1a,
                        x2);

  // calculate S0 from A0
  mbedtls_aes_crypt_ecb(&aes, MBEDTLS_AES_ENCRYPT, (const unsigned char*)a0,
                        s0);
  mbedtls_aes_free(&aes);

  // xor X2 S0
  for (int i = 0; i < 16; ++i) {
    t0[i] = x2[i] ^ s0[i];
  }

#ifdef DEBUG_PTM215
#ifdef DEBUG_ENCRYPTION
  log_d("## START encryption data ##");
  printBuf((byte*)switches[bleAddress].securityKey, 16, false, "sec key");
  printBuf((byte*)nonce, sizeof(nonce), false, "Nonce:");
  printBuf((byte*)a0, sizeof(a0), false, "A0   :");
  printBuf((byte*)b0, sizeof(b0), false, "B0   :");
  printBuf((byte*)b1, sizeof(b1), false, "B1   :");
  printBuf((byte*)x1, sizeof(x1), false, "X1   :");
  printBuf((byte*)x1a, sizeof(x1a), false, "X1A  :");
  printBuf((byte*)x2, sizeof(x2), false, "X2   :");
  printBuf((byte*)s0, sizeof(s0), false, "S2   :");
  printBuf((byte*)t0, sizeof(t0), false, "T0   :");
  log_d("## END encryption data ##");
#endif
#endif

  if (memcmp(t0, payload.data.receivedSecurityKey, 4) == 0) {
    return true;
  } else {
    log_e("Incorrect security key");
    return false;
  }
}

void Enocean_PTM215b::handleCommissioningPayload(NimBLEAddress& bleAddress, Payload& payload) {
  if (lastCommissioningCounter == payload.sequenceCounter) {
    // discard repeated messages
    return;
  }
  lastCommissioningCounter = payload.sequenceCounter;

  // Reverse order of bytes for NimBLEAddress
  byte addressBytes[6];
  for (uint8_t i = 0; i < 6; i++) {
    addressBytes[i] = payload.commissioning.staticSourceAddress[5-i];
  }

  BLEAddress address { addressBytes };
  #ifdef DEBUG_COMMISSIONING_DATA
    log_d("Commissioning event  - address: %s", address.toString().c_str());
    printBuf(payload.commissioning.securityKey, 16, false, "Security Key");
  #endif

  if (commissioningEventhandler) {
    CommissioningEvent event;
    event.address = address;
    memcpy(event.securityKey, payload.commissioning.securityKey, 16);
    commissioningEventhandler->handleEvent(event);
  }
}

void Enocean_PTM215b::registerBleSwitch(std::string bleAddress,
                                        const std::string securityKey,
                                        const uint8_t nodeIdA,
                                        const uint8_t nodeIdB,
                                        Eventhandler* handler) {
  registerBleSwitch(bleAddress, securityKey, nodeIdA, nodeIdA, nodeIdB, nodeIdB, handler);
}

void Enocean_PTM215b::registerBleSwitch(std::string bleAddress,
                                        const byte securityKey[16],
                                        const uint8_t nodeIdA,
                                        const uint8_t nodeIdB,
                                        Eventhandler* handler) {
  registerBleSwitch(bleAddress, securityKey, nodeIdA, nodeIdA, nodeIdB, nodeIdB, handler);
}

void Enocean_PTM215b::registerBleSwitch(
    const std::string bleAddress, const std::string securityKey,
    const uint8_t nodeIdA0, const uint8_t nodeIdA1, const uint8_t nodeIdB0,
    const uint8_t nodeIdB1, Eventhandler* handler) {
  
  byte key[16];
  hexStringToByteArray(securityKey, key, 16);
  registerBleSwitch(bleAddress, key, nodeIdA0, nodeIdA1, nodeIdB0, nodeIdB1, handler);
 }

void Enocean_PTM215b::registerBleSwitch(
    const std::string bleAddress, const byte securityKey[16],
    const uint8_t nodeIdA0, const uint8_t nodeIdA1, const uint8_t nodeIdB0,
    const uint8_t nodeIdB1, Eventhandler* handler) {
  Switch bleSwitch;
  bleSwitch.nodeIdA0     = nodeIdA0;
  bleSwitch.nodeIdA1     = nodeIdA1;
  bleSwitch.nodeIdB0     = nodeIdB0;
  bleSwitch.nodeIdB1     = nodeIdB1;
  bleSwitch.eventHandler = handler;
  memcpy(bleSwitch.securityKey, securityKey, 16);

  NimBLEAddress address{bleAddress};
  switches[address] = bleSwitch;

#ifdef DEBUG_PTM215
#ifdef DEBUG_REGISTER_CONFIG
  log_d("## START Register BLE switch ##");
  log_d("BLE address: %s", str_tolower(bleAddress).c_str());
  printBuf((byte*)bleSwitch.securityKey, sizeof(bleSwitch.securityKey), false, "Security key");
  log_d("NodeA0 id: %d", bleSwitch.nodeIdA0);
  log_d("NodeA1 id: %d", bleSwitch.nodeIdA1);
  log_d("NodeB0 id: %d", bleSwitch.nodeIdB0);
  log_d("NodeB1 id: %d", bleSwitch.nodeIdB1);
  log_d("## END Register BLE switch ##");
#endif
#endif
}

void Enocean_PTM215b::handleSwitchAction(const uint8_t switchStatus, NimBLEAddress& bleAddress) {

#ifdef DEBUG_PTM215
  log_d("handling button action: %d from %s", switchStatus,
        bleAddress.toString().c_str());
#endif

  if (switches.count(bleAddress) == 0) {
    log_w("No switch registered for address [%d]", bleAddress.toString().c_str());
    return;
  }

  //start repeat task
  suspendRepeatTask(false);

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
    suspendRepeatTask(true);
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
    suspendRepeatTask(true);
    return;
  }

  ActionType type = (ActionType)(switchStatus & 0x01);

  SwitchAction action;
  action.nodeId       = nodeId;
  action.actionType   = type;
  action.direction    = direction;
  action.eventHandler = bleSwitch.eventHandler;

  SwitchEvent event;
  event.nodeId    = nodeId;
  event.direction = direction;

  if (type == ActionType::Release) {
    suspendRepeatTask(true);
    if (millis() - INITIAL_REPEAT_WAIT < actions[nodeId].pushStartTime) {
      event.eventType = EventType::ReleaseShort;
    } else {
      event.eventType = EventType::ReleaseLong;
    }
    bleSwitch.eventHandler->handleEvent(event);
    actions.erase(nodeId);
  } else {
    action.pushStartTime = millis();
    actions[nodeId]      = action;
    event.eventType      = EventType::Pushed;
    bleSwitch.eventHandler->handleEvent(event);
  }
}

void Enocean_PTM215b::generateRepeatEvents() {
  for (auto const& pair : actions) {
    SwitchAction action = pair.second;
    if (millis() - INITIAL_REPEAT_WAIT > action.pushStartTime) {
      SwitchEvent event;
      event.nodeId    = action.nodeId;
      event.direction = action.direction;
      event.eventType = EventType::Repeat;
      action.eventHandler->handleEvent(event);
    }
  }
}

} // namespace PTM215b