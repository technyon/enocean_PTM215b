#include "EnOceanPTM215EventAdapter.h"
#include "esp_task_wdt.h"
#include "mbedtls/aes.h"
#include <algorithm>

namespace EnOcean {

void repeatEventsTask(void* pvParameters) {
#ifdef DEBUG_ENOCEAN
  log_d("TASK: PTM215b repeatEvents task started on core: %d", xPortGetCoreID());
#endif
  esp_task_wdt_add(NULL);
  PTM215EventAdapter* adapter = static_cast<PTM215EventAdapter*>(pvParameters);

  while (1) {
    esp_task_wdt_reset();
    delay(REPEAT_INTERVAL);
    adapter->generateRepeatEvents();
  }
}

PTM215EventAdapter::~PTM215EventAdapter() {
  if (repeatEventsTaskHandle) {
    vTaskDelete(repeatEventsTaskHandle);
  }
}

void PTM215EventAdapter::handlePayload(Device& device, Payload& payload) {
  if (securityKeyValid(device, payload)) {
    PTM215Event ptm215Event = mapToPTM215Event(device, payload);
    manageEventList(ptm215Event);

    device.handler->handleEvent(DeviceType::PTM215B, (void*)&ptm215Event);
  }
}

void PTM215EventAdapter::manageEventList(PTM215Event& event) {
  NimBLEAddress address = event.device->address;
  if (event.eventType == EventType::Pushed) {
    lastEvents[address]  = event;
  } else if ((event.eventType == EventType::ReleaseShort) || (event.eventType == EventType::ReleaseLong)) {
    lastEvents.erase(address);
  }

  if (lastEvents.size() > 0) {
    startRepeatTask();
  } else {
    suspendRepeatTask();
  }

}

void PTM215EventAdapter::startRepeatTask() {
  if (!repeatEventsTaskHandle) {
    xTaskCreatePinnedToCore(&repeatEventsTask, "PMT215RepeatTask", 4096, this, 1, &repeatEventsTaskHandle, CONFIG_BT_NIMBLE_PINNED_TO_CORE);
  } 

  if (isRepeatTaskSuspended()) {
    vTaskResume(repeatEventsTaskHandle);
    esp_task_wdt_add(repeatEventsTaskHandle);
  }
}

void PTM215EventAdapter::suspendRepeatTask() {
  if (repeatEventsTaskHandle != NULL) {
    esp_task_wdt_delete(repeatEventsTaskHandle);
    vTaskSuspend(repeatEventsTaskHandle);
  }
}

bool PTM215EventAdapter::isRepeatTaskSuspended() {
  if (repeatEventsTaskHandle) {
    eTaskState state = eTaskGetState(repeatEventsTaskHandle);
    return ((state == eSuspended) || (state == eDeleted));
  }
  return false;
}

bool PTM215EventAdapter::securityKeyValid(Device& device, Payload& payload) {
  unsigned char nonce[13] = {0};
  uint8_t a0Flag          = 0x01;
  uint8_t b0Flag          = 0x49;
  uint16_t inputLength    = 9;
  unsigned char a0[16]    = {0};
  unsigned char b0[16]    = {0};
  unsigned char b1[16]    = {0};

  std::string bleAddressString = device.address.toString();

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
  b1[10] = payload.data.raw[0];

  unsigned char x1[16]  = {0};
  unsigned char x1a[16] = {0};
  unsigned char x2[16]  = {0};
  unsigned char s0[16]  = {0};
  unsigned char t0[16]  = {0};

  mbedtls_aes_context aes;
  mbedtls_aes_init(&aes);

  mbedtls_aes_setkey_enc(
      &aes, (const unsigned char*)device.securityKey, sizeof(device.securityKey) * 8);

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

  if (memcmp(t0, payload.data.signature, 4) == 0) {
    return true;
  } else {
    log_e("Incorrect signature");
    return false;
  }
}

PTM215Event PTM215EventAdapter::mapToPTM215Event(Device& device, Payload& payload) {
  PTM215Event event;
  uint8_t switchStatus = payload.data.raw[0];

  Direction direction;
  if (switchStatus & 0b00001010) {
    direction = Direction::Up;
  } else { 
    direction = Direction::Down;
  }
  
  event.device = &device;
  event.direction = direction;

  if ((switchStatus & 0x01) == 0) { // release
    if ((lastEvents.count(device.address) == 0) || (millis() - INITIAL_REPEAT_WAIT < lastEvents[device.address].pushStartTime)) {
      event.eventType = EventType::ReleaseShort;
    } else {
      event.eventType = EventType::ReleaseLong;
    }
  } else { // push
    event.eventType      = EventType::Pushed;
    event.pushStartTime = millis();
  }

  return event;
}

void PTM215EventAdapter::generateRepeatEvents() {
  for (auto& pair : lastEvents) {
    PTM215Event event = pair.second;
    if (millis() - INITIAL_REPEAT_WAIT > event.pushStartTime) {
      pair.second.eventType = EventType::Repeat;
      event.device->handler->handleEvent(DeviceType::PTM215B, (void*)&pair.second);
    }
  }
}

} // namespace EnOcean
