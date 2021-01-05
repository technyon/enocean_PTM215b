# Enocean_PTM215b

This library is work in progress but following features are working:
-Read switch config from json file handling multiple switches
-Detect/handle long and short press
-Security checks on payload length, static source address, GAP AD Type and security key encryption using AES128

Hardware supported:
-Tested on an ESP32wroom
-Enocean PTM215B batteryless BLE switch (only tested with 1 switch so far)

Implemented using https://www.enocean.com/en/products/enocean_modules_24ghz_ble/ptm-215b/user-manual-pdf/
