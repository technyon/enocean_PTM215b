/**
 * A BLE client example to receive events from an Enocean PTM215b switch
 *
 *
 * author Jeroen
 */

#include "enocean_PTM215b.h"

#define BLE_ADDRESS "e2:15:00:01:72:f9"
#define SECURITY_KEY "1CB95CE63F19ADC7E0FB92DA56D69219"

class EH : public PTM215b::Eventhandler {
public:
    EH() {};
    virtual ~EH() {};

    void handleEvent(PTM215b::SwitchEvent& evt) override {

        std::string type;
        switch (evt.eventType) {
            case PTM215b::EventType::Pushed:
                type = "Pushed";
                break;
            case PTM215b::EventType::Repeat:
                type = "Repeat";
                break;
            case PTM215b::EventType::ReleaseLong:
                type = "ReleasedLong";
                break;
            case PTM215b::EventType::ReleaseShort:
                type = "ReleasedShort";
                break;
            default:
                break;
        }

        std::string direction = (evt.button == PTM215b::Button::A_UP || evt.button == PTM215b::Button::B_UP) ? "Up" : "Down";

        log_d("BleSwitchEvent Received: Node Id: %d, Type: %s, Direction: %s", evt.nodeId, type.c_str(), direction.c_str());
    };

};

EH handler;
PTM215b::Enocean_PTM215b enocean_PTM215b(handler, true);

void setup(){
    Serial.begin(115200);

    BLEDevice::init("ESP32_client");
    enocean_PTM215b.initialize();

    enocean_PTM215b.registerBleSwitch(BLE_ADDRESS, SECURITY_KEY, 0, 1, 2 , 3);
}

void loop(){
    delay(1000);
}