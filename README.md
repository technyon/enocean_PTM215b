# Enocean_PTM215b

This library provides an implementation for an EnOcean PTM215b selfpowered BLE switch on a ESP32 based MCU

https://www.enocean.com/en/products/enocean_modules_24ghz_ble/ptm-215b/


## Hardware
Hardware supported:
* Tested on an ESP32wroom
* Enocean PTM215B selfpowered BLE switch (only tested with 1 switch so far)


## Development
This library is work in progress but following features are working:
* Register a PTM switch by BLE address (**"MAC Address"**), specifying a 32 character **Security Key** and seperate **NodeIds** for rocker A and B
* Task based detection of `Pressed`, `Repeat` and `Released` events
* Security checks on payload length, source address, GAP AD Type and security key encryption using AES128
* Dispatch events received through BLE to external event handler
* Dispatch `Repeat` events to external event handler every 1000ms, if button is pushed and held down

Current implementation is based on https://www.enocean.com/en/products/enocean_modules_24ghz_ble/ptm-215b/user-manual-pdf/

All classes are defined within the namespace `PTM215b`.


## Usage

* Add dependency to `I-Connect/enocean_PTM215b` to your project
* `#include "enocean_PMT215b.h"`
* Define an Eventhandler class implementing the `PTM215b::Eventhandler` interface
* Create an instance of the derived Eventhandler class
* Create an instance of the `Enocean_PTM215b` class, providing the Eventhandler instance as constructor parameter
* Initialize BLEDevice
* Register switches with `BLEAddress`, 32 character `SecurityKey`, `NodeIdA` for rocker A and `NodeIdB` for rocker B
* Call `enocean_PTM215b::initialize()`

On every switch event, a call will be made to `EventHandler.handleEvent()` with a `PTM215b::BleSwitchEvent` as parameter.

    struct BleSwitchEvent {
      uint8_t nodeId;         // Id used when registering the switch
      Direction direction;    // Up or Down
      EventType eventType;    // Pushed, Repeat, Released
      uint32_t pushStartTime; // millis() timestamp on moment of last Pushed event
    };


The `main.cpp` file contains an example of how to use a `.json` config file to define the properties of the switches used

## Todo
Future implementations might include:
* Handling events with multiple buttons pushed or held
* Make timing of Repeat events configurable per rocker
* Allow for other components to also receive BLEScan results (daisy-chaining)
* Implement handling optional data in DataPayload


