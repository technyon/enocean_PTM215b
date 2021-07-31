# EnOcean

This library provides an implementation for EnOcean selfpowered BLE devices on a ESP32 based MCU

https://www.enocean.com/en/products

## Hardware
Hardware supported:
* Tested on an ESP32wroom
* EnOcean PTM215B selfpowered BLE switch


## Development
This library is work in progress and currently includes:
* Register a PTM switch by BLE address (**"MAC Address"**), specifying a 32 character **Security Key** and seperate **NodeIds** for either rocker A and B
or specify separate **NodeIds** for the four buttons A0, A1, B0 and B1
* Task based detection of `Pushed`, `Repeat`, `ReleaseShort` and `ReleaseLong` events
* Security checks on payload length, source address, GAP AD Type and security key encryption using AES128
* Dispatch events received through BLE to external event handler
* Dispatch `Repeat` events to external event handler every 1000ms, if button is pushed and held down

All classes are defined within the namespace `EnOcean`.

Current implementation for PTM215b switches is based on `PTM_215B_User_Manual_v2.1.pdf` included in the `documentation` folder


## PTM215 Events
`Pushed` event is generated when a PTM215 button is pushed.
If the button is held down, `Repeat` events will be generated every 500 ms after an initial delay.
When the button is released without generating `Repeat` events, then a `ReleaseShort` event is generated,
otherwise a `ReleaseLong` event is generated.

## Usage for PTM215 switches

* Add dependency to `I-Connect/EnOcean` to your project
* Add `#include "EnOceanBLEScanner.h"`
* Create an instance of the `EnOcean::BLEScanner` class
* Define a class implementing `EnOcean::PTM215EventHandler` and create an instance
* Initialize BLEDevice
* Register switches with `BLEAddress`, 32 character `SecurityKey`, **NodeIds** and a pointer to the eventhandler
* Call `bleScanner.initialize()`

On every valid switch event from a registered BLEAddress, a call will be made to the `handleEvent()` method of the eventHandler registered with the switch with a `PTM215b::SwitchEvent` as parameter.

The example `basicWthConfigFile.cpp` file contains an example of how to use a `.json` config file to define the properties of the switches used, using the ArduinoJson library


##Debugging
If `DEBUG_ENOCEAN` is defined, log messages will be output to Serial for debugging

## Todo
Future implementations might include:
* Implement STM550 multiswitch and EMDCB motion detector devices
* Handling events with multiple buttons pushed or held
* Make timing of Repeat events configurable per rocker
* Implement handling optional data in DataPayload


