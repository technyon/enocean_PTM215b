# EnOcean_PTM215b

This library provides an implementation for an EnOcean PTM215b selfpowered BLE switch on a ESP32 based MCU

https://www.enocean.com/en/products/enocean_modules_24ghz_ble/ptm-215b/


## Hardware
Hardware supported:
* Tested on an ESP32wroom
* EnOcean PTM215B selfpowered BLE switch


## Development
This library is work in progress but following features are working:
* Register a PTM switch by BLE address (**"MAC Address"**), specifying a 32 character **Security Key** and seperate **NodeIds** for either rocker A and B
or specify separate **NodeIds** for the four buttons A0, A1, B0 and B1
* Task based detection of `Pressed`, `Repeat`, `ReleasedShort` and `ReleasedLong` events
* Security checks on payload length, source address, GAP AD Type and security key encryption using AES128
* Dispatch events received through BLE to external event handler
* Dispatch `Repeat` events to external event handler every 1000ms, if button is pushed and held down

Current implementation is based on https://www.enocean.com/en/products/enocean_modules_24ghz_ble/ptm-215b/user-manual-pdf/

All classes are defined within the namespace `PTM215b`.


## Usage

* Add dependency to `I-Connect/enocean_PTM215b` to your project
* Add `#include "enocean_PMT215b.h"`
* Create an instance of the derived Eventhandler class
* Create an instance of the `EnOcean_PTM215b` class
* Initialize BLEDevice
* Define an Eventhandler class implementing the `PTM215b::Eventhandler` interface
* Register switches with `BLEAddress`, 32 character `SecurityKey`, **NodeIds** and a pointer to the eventhandler
* Call `enocean_PTM215b::initialize()`

On every switch event, a call will be made to the `handleEvent()` method of the eventHandler registered with the switch with a `PTM215b::SwitchEvent` as parameter.

    struct SwitchEvent {
      uint8_t nodeId;         // Id used when registering the switch
      Direction direction;    // Up or Down
      EventType eventType;    // Pushed, Repeat, ReleasedLong, ReleaseShort
    };


The example `basicWthConfigFile.cpp` file contains an example of how to use a `.json` config file to define the properties of the switches used, using the ArduinoJson library

## Todo
Future implementations might include:
* Handling events with multiple buttons pushed or held
* Make timing of Repeat events configurable per rocker
* Allow for other components to also receive BLEScan results (daisy-chaining)
* Implement handling optional data in DataPayload


