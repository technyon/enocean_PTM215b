
#include "EnOceanBLEScanner.h"
#include "EnOceanPTM215EventAdapter.h"
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

void hexStringToByteArray(std::string stringInput, byte* output, uint8_t byteLength) {
  for (uint8_t i = 0; i < byteLength; i++) {
    output[i] = strtol(stringInput.substr(i * 2, 2).c_str(), NULL, 16);
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

bool BLEScanner::isSuspended(TaskHandle_t taskHandle) {
  if (taskHandle) {
    eTaskState state = eTaskGetState(taskHandle);
    return ((state == eSuspended) || (state == eDeleted));
  }
  return false;
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

void BLEScanner::handleDataPayload(NimBLEAddress& bleAddress, Payload& payload) {
  if (activeCommissioningAddress == bleAddress) {
    // Data event received from active commissioning address -> end commissioning mode
    activeCommissioningAddress = NimBLEAddress();
  }

  if (devices.count(bleAddress)) {
    Device device = devices[bleAddress];
    if (device.lastSequenceCounter < payload.sequenceCounter) {
      devices[bleAddress].lastSequenceCounter = payload.sequenceCounter;
      switch (payload.deviceType) {
        case DeviceType::PTM215B: {
          // Note that devices address is stored for repeat events, so don't use local var device
          ptm215Adapter.handlePayload(devices[bleAddress], payload);
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
  devices[address] = device;
  return device;
}

void BLEScanner::registerPTM215Device(const std::string bleAddress, const std::string securityKey, PTM215EventHandler* handler,
                                      bool buttonA0, bool buttonA1, bool buttonB0, bool buttonB1) {
  Device device = registerDevice(bleAddress, securityKey);
  ptm215Adapter.registerHandler(device, handler, buttonA0, buttonA1, buttonB0, buttonB1);
}

void BLEScanner::unRegisterAddress(const NimBLEAddress address) {
  devices.erase(address);
}

DeviceType BLEScanner::getTypeFromAddress(const NimBLEAddress& address) {
  const uint8_t* nativeAddressLSB = address.getNative(); // LSB
  uint8_t nativeAddress[6];
  std::reverse_copy(nativeAddressLSB, nativeAddressLSB + 6, nativeAddress);

  if (memcmp(nativeAddress, STM550B_PREFIX_ADDRESS, 2) == 0) {
    return DeviceType::STM550B;
  }

  if (memcmp(nativeAddress, EMDCB_PREFIX_ADDRESS, 2) == 0) {
    return DeviceType::EMDCB;
  }

  if (memcmp(nativeAddress, PTM215B_PREFIX_ADDRESS, 2) == 0) {
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