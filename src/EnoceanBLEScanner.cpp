
#include "EnOceanBLEScanner.h"
#include "EnOceanUtils.h"
#include "esp_task_wdt.h"
#include "mbedtls/aes.h"
#include <algorithm>

// #define CONFIG_BT_NIMBLE_PINNED_TO_CORE 1

namespace EnOcean {

void bleScanTask(void* pvParameters) {
#ifdef DEBUG_ENOCEAN
  log_d("TASK: EnOcean BLE scan task started on core: %d", xPortGetCoreID());
#endif
  //TODO implement watchdog? Is triggered by the blocking start call below
  BLEScanner* scanner  = static_cast<BLEScanner*>(pvParameters);
  NimBLEScan* pBLEScan = NimBLEDevice::getScan();        // get global scan-object
  pBLEScan->setAdvertisedDeviceCallbacks(scanner, true); // want duplicates as we will not be connecting and only listening to adv data
  pBLEScan->setActiveScan(true);                         // active scan uses more power, but get results faster
  pBLEScan->setInterval(17);
  pBLEScan->setWindow(17); // Must be between 4 and 16K and smaller or equal to Interval
  esp_task_wdt_add(NULL);
  while (1) {
    esp_task_wdt_reset();
    pBLEScan->start(1, true);
    pBLEScan->clearResults();
  }
}


BLEScanner::BLEScanner() {
}

BLEScanner::~BLEScanner() {
  if (bleScanTaskHandle)
    vTaskDelete(bleScanTaskHandle);
}

void BLEScanner::initialize() {
  if (!NimBLEDevice::getInitialized()) {
    log_e("BLEDevice not initialized");
    return;
  }
  startTasks();
}

void BLEScanner::startTasks() {
  xTaskCreatePinnedToCore(&bleScanTask, "EnOceanScanTask", 4096, this, 1, &bleScanTaskHandle, CONFIG_BT_NIMBLE_PINNED_TO_CORE);
}

bool BLEScanner::isSuspended() {
  eTaskState state = eTaskGetState(bleScanTaskHandle);
  return ((state == eSuspended) || (state == eDeleted));
}

void BLEScanner::setScanTaskPriority(uint8_t prio) {
  vTaskPrioritySet(bleScanTaskHandle, prio);
}

void BLEScanner::onResult(NimBLEAdvertisedDevice* advertisedDevice) {
  Payload payload = getPayload(advertisedDevice);

  if ((payload.type != ENOCEAN_PAYLOAD_TYPE) || (memcmp(payload.manufacturerId, ENOCEAN_PAYLOAD_MANUFACTURER, sizeof(ENOCEAN_PAYLOAD_MANUFACTURER)) != 0)) {
    return;
  }

#ifdef DEBUG_ENOCEAN
  log_d("EnOcean event received from %s", advertisedDevice->getAddress().toString().c_str());
#endif

  NimBLEAddress bleAddress = advertisedDevice->getAddress();
  if (payload.payloadType == PayloadType::Commissioning) {
    handleCommissioningPayload(bleAddress, payload);
  } else {
    handleDataPayload(bleAddress, payload);
  }
}

Payload BLEScanner::getPayload(NimBLEAdvertisedDevice* advertisedDevice) {
  Payload payload;
  memset(&payload, 0x00, sizeof(payload));

  // Pointer to first byte of payload to read next
  uint8_t* nextPayload = advertisedDevice->getPayload();

  // Read Len, Type, Manufacturer and counter
  memcpy(&payload, advertisedDevice->getPayload(), 8);
  nextPayload = nextPayload + 8;

  payload.deviceType = getTypeFromAddress(advertisedDevice->getAddress());
  if (payload.deviceType == DeviceType::UNKNOWN) {
    // Return incomplete payload, should be handled in caller
    return payload;
  }

  if (payload.len == 29) {
    payload.payloadType = PayloadType::Commissioning;
    memcpy(&payload.commissioning, nextPayload, payload.len - 7);
  } else {
    uint8_t dataLen     = payload.len - 11;
    payload.payloadType = PayloadType::Data;
    memcpy(&payload.data.raw, nextPayload, dataLen);
    memcpy(&payload.data.signature, nextPayload + dataLen, 4);
  }
  return payload;
}

bool BLEScanner::securityKeyValid(Device& device, Payload& payload) {
  unsigned char nonce[13] {0};
  uint8_t a0Flag          = 0x01;
  uint8_t b0Flag          = 0x49;
  uint16_t inputLength    = payload.len + 1; // include the length byte 
  unsigned char a0[16]    {0};
  unsigned char b0[16]    {0};
  unsigned char b1[16]    {0};
  unsigned char b2[16]    {0};

  // construct nonce
  memcpy(nonce, device.address.getNative(), 6);
  memcpy(&nonce[6], &payload.sequenceCounter, sizeof(payload.sequenceCounter));


  // construct a0 input parameter
  a0[0] = a0Flag;
  memcpy(&a0[1], nonce, sizeof(nonce));

  // construct b0 input parameter
  b0[0] = b0Flag;
  memcpy(&b0[1], nonce, sizeof(nonce));

  // construct b1 input parameter
  b1[0] = ((inputLength - 4) >> (8 * 1)) & 0xff; // exclude signature
  b1[1] = ((inputLength - 4) >> (8 * 0)) & 0xff;
  b1[2] = payload.len;
  b1[3] = payload.type;
  memcpy(&b1[4], payload.manufacturerId, sizeof(payload.manufacturerId));
  memcpy(&b1[6], &payload.sequenceCounter, sizeof(payload.sequenceCounter));

  if (inputLength <= 18) {
    memcpy(&b1[10], payload.data.raw, inputLength - 12);  
  } else {
    memcpy(&b1[10], payload.data.raw, 6);
    memcpy(b2, payload.data.raw + 6, inputLength - 18);
  }

  unsigned char x1[16]  {0};
  unsigned char x1a[16] {0};
  unsigned char x2[16]  {0};
  unsigned char x2a[16] {0};
  unsigned char x3[16]  {0};
  unsigned char s0[16]  {0};
  unsigned char t0[16]  {0};

  mbedtls_aes_context aes;
  mbedtls_aes_init(&aes);

  mbedtls_aes_setkey_enc(&aes, (const unsigned char*)device.securityKey, sizeof(device.securityKey) * 8);

  // calculate X1 from B0
  mbedtls_aes_crypt_ecb(&aes, MBEDTLS_AES_ENCRYPT, b0, x1);


  // xor X1 B1
  for (int i = 0; i < 16; ++i) {
    x1a[i] = x1[i] ^ b1[i];
  }

  // calculate X2 from X1A
  mbedtls_aes_crypt_ecb(&aes, MBEDTLS_AES_ENCRYPT, x1a, x2);

  if (inputLength > 18) {
    // Calculate X2A
    for (int i = 0; i < 16; ++i) {
      x2a[i] = x2[i] ^ b2[i];
    }

    // calculate X3 from X2A
    mbedtls_aes_crypt_ecb(&aes, MBEDTLS_AES_ENCRYPT, x2a, x3);
  }

  // calculate S0 from A0
  mbedtls_aes_crypt_ecb(&aes, MBEDTLS_AES_ENCRYPT, a0, s0);
  mbedtls_aes_free(&aes);

  if (inputLength > 18) {
    // xor X3 S0
    for (int i = 0; i < 16; ++i) {
      t0[i] = x3[i] ^ s0[i];
    }
  } else {
    for (int i = 0; i < 16; ++i) {
      t0[i] = x2[i] ^ s0[i];
    }
  }

  // printBuffer(nonce, 13, false, "Nonce\t");
  // printBuffer(a0, 16, false, "A0\t");
  // printBuffer(b0, 16, false, "B0\t");
  // printBuffer(b1, 16, false, "B1\t");
  // printBuffer(b2, 16, false, "B2\t");
  // printBuffer(x1, 16, false, "X1\t");
  // printBuffer(x1a, 16, false, "X1a\t");
  // printBuffer(x2, 16, false, "X2\t");
  // printBuffer(x2a, 16, false, "X2a\t");
  // printBuffer(x3, 16, false, "X3\t");
  // printBuffer(s0, 16, false, "S0\t");
  // printBuffer(payload.data.signature, 4, false, "Signature");
  // printBuffer(t0, 4, false, "T0\t");

  if (memcmp(t0, payload.data.signature, 4) == 0) {
    return true;
  } else {
    log_e("Incorrect signature");
    return false;
  }
}

void BLEScanner::handleDataPayload(NimBLEAddress& bleAddress, Payload& payload) {
  if (activeCommissioningAddress == bleAddress) {
    // Data event received from active commissioning address -> end commissioning mode
    activeCommissioningAddress = NimBLEAddress();
  }

  if (devices.count(bleAddress)) {
    Device& device = devices[bleAddress];
    if ((device.lastSequenceCounter < payload.sequenceCounter) && securityKeyValid(device, payload)) {
      device.lastSequenceCounter = payload.sequenceCounter;
      switch (payload.deviceType) {
        case DeviceType::PTM215B: {
          ptm215Adapter.handlePayload(device, payload);
          break;
        }
        case DeviceType::EMDCB: {
          emdcbAdapter.handlePayload(device, payload);
          break;
        }

        case DeviceType::STM550B: {
          stm550Adapter.handlePayload(device, payload);
          break;
        }

        default: {
          log_w("Devicetype [%d] adapter not implemented", payload.deviceType);
          break;
        }
      }
    }
  }
}

void BLEScanner::handleCommissioningPayload(NimBLEAddress& bleAddress, Payload& payload) {
  if ((uint64_t)activeCommissioningAddress == 0) {
    activeCommissioningAddress = bleAddress;
  } else if (activeCommissioningAddress != bleAddress) {
    log_w("Ignored commissioning for %s, already active for %s", bleAddress, activeCommissioningAddress);
    return;
  }

  if (lastCommissioningCounter >= payload.sequenceCounter) {
    // discard repeated messages
    return;
  }

  if (commissioningEventhandler) {
    lastCommissioningCounter = payload.sequenceCounter;
    // Reverse order of bytes for NimBLEAddress
    byte addressBytes[6];
    for (uint8_t i = 0; i < 6; i++) {
      addressBytes[i] = payload.commissioning.staticSourceAddress[5 - i];
    }

    NimBLEAddress address{addressBytes};

    CommissioningEvent event;
    event.address = address;
    event.type    = getTypeFromAddress(address);
    memcpy(event.securityKey, payload.commissioning.securityKey, 16);
    event.securityKey[16] = 0; // Add char terminator
    commissioningEventhandler->handleEvent(event);
  }
}

Device BLEScanner::registerDevice(const std::string bleAddress, const std::string securityKey) {
  byte key[16];
  hexStringToByteArray(securityKey, key, 16);
  return registerDevice(bleAddress, key);
}

Device BLEScanner::registerDevice(const std::string bleAddress, const byte securityKey[16]) {
  Device device;
  memcpy(device.securityKey, securityKey, 16);
  NimBLEAddress address{bleAddress};
  device.address   = address;
  device.type      = getTypeFromAddress(address);
  devices[address] = device;
  return device;
}

void BLEScanner::registerPTM215Device(const std::string bleAddress, const std::string securityKey, const uint8_t eventHandlerNodeId,
                          bool buttonA0, bool buttonA1, bool buttonB0, bool buttonB1) {
  Device device = registerDevice(bleAddress, securityKey);
  ptm215Adapter.registerHandler(device, eventHandlerNodeId, buttonA0, buttonA1, buttonB0, buttonB1);
}

void BLEScanner::registerPTM215Device(const std::string bleAddress, const std::string securityKey, PTM215EventHandler* handler,
                                      bool buttonA0, bool buttonA1, bool buttonB0, bool buttonB1) {
  Device device = registerDevice(bleAddress, securityKey);
  ptm215Adapter.registerHandler(device, handler, buttonA0, buttonA1, buttonB0, buttonB1);
}

void BLEScanner::registerEMDCBDevice(const std::string bleAddress, const std::string securityKey, EMDCBEventHandler* handler) {
  Device device = registerDevice(bleAddress, securityKey);
  emdcbAdapter.registerHandler(device, handler);
}

void BLEScanner::registerSTM550BDevice(const std::string bleAddress, const std::string securityKey, STM550BEventHandler* handler) {
  Device device = registerDevice(bleAddress, securityKey);
  stm550Adapter.registerHandler(device, handler);
}

void BLEScanner::unRegisterAddress(const NimBLEAddress address) {
  devices.erase(address);
}

DeviceType BLEScanner::getTypeFromAddress(const NimBLEAddress& address) {
  const uint8_t* nativeAddressLSB = address.getNative(); // LSB
  uint8_t nativeAddress[6];
  std::reverse_copy(nativeAddressLSB, nativeAddressLSB + 6, nativeAddress);

  if (memcmp(nativeAddress, STM550B_PREFIX_ADDRESS, sizeof(STM550B_PREFIX_ADDRESS)) == 0) {
    return DeviceType::STM550B;
  }

  if (memcmp(nativeAddress, EMDCB_PREFIX_ADDRESS, sizeof(EMDCB_PREFIX_ADDRESS)) == 0) {
    return DeviceType::EMDCB;
  }

  if (memcmp(nativeAddress, PTM215B_PREFIX_ADDRESS, sizeof(PTM215B_PREFIX_ADDRESS)) == 0) {
    if ((nativeAddress[2] & 0xF0) == 1) {
      return DeviceType::PTM535BZ;
    }

    if ((nativeAddress[2] & 0xF0) == 0) {
      return DeviceType::PTM215B;
    }
  }
  return DeviceType::UNKNOWN;
}

} // namespace EnOcean