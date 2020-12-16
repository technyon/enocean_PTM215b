/**
 * A BLE client example to receive events from an Enocean PTM215b switch
 * author Jeroen
 */

#include "enocean_PTM215b.h"

Enocean_PTM215b enocean_PTM215b;

void setup(){
    Serial.begin(115200);
    log_d("Starting Enocean_PTM215b BLE Client application...");
    
    enocean_PTM215b.initialize();
}

void loop(){

    delay(1000);

}