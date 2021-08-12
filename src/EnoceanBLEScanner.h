#pragma once

#include "Arduino.h"
#include "EnOceanConstants.h"
#include "EnOceanDataTypes.h"
#include "PTM215/EnOceanPTM215EventAdapter.h"
#include "EMDCB/EnOceanEMDCBEventAdapter.h"
#include "STM550B/EnOceanSTM550BEventAdapter.h"
#include "NimBLEDevice.h"
#include <map>


namespace EnOcean {

/**
 * @brief Class handling BLE advertisement messages received from multiple
 * EnOcean devices
 *
 * The class works by starting a background taks which will scan for BLE
 * advertising events. On receipt of an event the onResult() method will be called
 * which wil validate the message and dispatch it to an eventAdapater based on the 
 * device type where the message came from
 *
 * EnOcean devices need to be registered to this class using registerXXXDevice() methods,
 * which links the BLE address to a handler which will be called on
 * reception of an event on that BLE address
 *
 * Before calling initialize() to create and start the tasks, the global
 * BLEDevice must have been initialized with BLEDevice::init()
 */
class BLEScanner : public BLEAdvertisedDeviceCallbacks {
public:
  BLEScanner();
  virtual ~BLEScanner();

  /**
   * @brief Set BLE Scan task priorities
   *
   */
  void setScanTaskPriority(uint8_t prio);

  /**
   * @brief return if BLE Scan task is suspended
   *
   * @param taskHandle taskHandle to check
   */
  bool isSuspended();

  /**
   * @brief Initialize object and start background tasks
   *
   * The BLEDevice must have been initialized, otherwise an error will be logged
   * and the background will not be started This must be called before using the
   * switches
   */
  void initialize();

  void setCommissioningEventHandler(CommissioningEventhandler* handler) {
    commissioningEventhandler = handler;
  }

  /**
   * @brief Register an EnOcean PTM215 device (Switches)
   *
   * @param bleAddress BLE address of switch being handled
   * @param securityKey Security key retrieved from QR code, NFC or commissioning data
   * @param handler PayloadHandler that will be called on receipt of an event on the bleAddress
   * @param buttonXX booleans indicating which of the buttons A0, A1, B0 and B1 will be handled by this handler
   */
  void registerPTM215Device(const std::string bleAddress, const std::string securityKey, PTM215EventHandler* handler,
                            bool buttonA0, bool buttonA1, bool buttonB0, bool buttonB1);
  void registerPTM215Device(const std::string bleAddress, const std::string securityKey, const uint8_t eventHandlerNodeId,
                            bool buttonA0, bool buttonA1, bool buttonB0, bool buttonB1);

  /**
   * @brief Register an EnOcean EMDCB device (motion detector)
   *
   * @param bleAddress BLE address of device being handled
   * @param securityKey Security key retrieved from QR code, NFC or commissioning data
   * @param handler PayloadHandler that will be called on receipt of an event on the bleAddress
   */ 
  void registerEMDCBDevice(const std::string bleAddress, const std::string securityKey, EMDCBEventHandler* handler);

  /**
   * @brief Register an EnOcean STM550B device (multi sensor)
   *
   * @param bleAddress BLE address of device being handled
   * @param securityKey Security key retrieved from QR code, NFC or commissioning data
   * @param handler PayloadHandler that will be called on receipt of an event on the bleAddress
   */ 
  void registerSTM550BDevice(const std::string bleAddress, const std::string securityKey, STM550BEventHandler* handler);

  void unRegisterAddress(const NimBLEAddress address);

  // public for testing
  bool securityKeyValid(Device& device, Payload& payload);

private:
  TaskHandle_t bleScanTaskHandle                       = nullptr;
  CommissioningEventhandler* commissioningEventhandler = nullptr;
  uint32_t lastCommissioningCounter                    = 0;
  /**
   * @brief Address of currently active commissioning event
   * 
   * If 0 then no commissioning is active
   */
  NimBLEAddress activeCommissioningAddress{}; 

  PTM215EventAdapter ptm215Adapter;
  EMDCBEventAdapter emdcbAdapter;
  STM550BEventAdapter stm550Adapter;

  Device registerDevice(const std::string bleAddress, const std::string securityKey);
  Device registerDevice(const std::string bleAddress, const byte securityKey[16]);
  /**
   * @brief Map of registered devices by BleAddress
   */
  std::map<NimBLEAddress, Device> devices;

  /**
   * @brief Create queue and start BLE scan task
   *
   */
  void startTasks();

  /**
   * @brief Implementation of BLEAdvertisedDeviceCallbacks to handle advertisement events. Checks
   * manufacturer specific data if it is an EnOcean switch and calls payloadHandler() of the
   * registered PayloadHandler 
   *
   * @param advertisedDevice Holds BLE address and payload
   */
  void onResult(BLEAdvertisedDevice* advertisedDevice) override;

  /**
   * @brief Construct the Payload object from the data in advertisedDevice
   * 
   * @param advertisedDevice 
   * @return Payload 
   */
  Payload getPayload(NimBLEAdvertisedDevice* advertisedDevice);

  /**
   * @brief Dedupes messages, Checks sequence counter and security key
   * and calls handlePayload() of the EventAdapter of the device type
   *
   * @param bleAddress 
   * @param payload
   */
  void handleDataPayload(NimBLEAddress& bleAddress, Payload& payload);

  /**
   * @brief Handles commissioning data
   *
   * @param bleAddress BLE address of switch sending commissioning data
   * @param payload
   */
  void handleCommissioningPayload(NimBLEAddress& bleAddress, Payload& payload);

  DeviceType getTypeFromAddress(const NimBLEAddress& address);
};

} // namespace EnOcean