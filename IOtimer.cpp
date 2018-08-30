#include "IOtimer.h"
#include "common.h"
#include <Arduino.h>

IOtimer::IOtimer(int pin, char jar)
    : IO(pin), jarNum(jar), dTimeSet(0), dOnTime(0), dState(LOW), lTimeSet(0),
      lOnTime(0), lState(LOW) {

    pinMode(IO, OUTPUT);
    // Initialize each valve IO High to set all relays Low.
    digitalWrite(IO,dState); 
}

void IOtimer::Update() {
    if (dState == HIGH && now >= dTimeSet + dOnTime) {
        // IF jar is on and onTime has elapsed, turn jar and pump off.
        dState = LOW;
        digitalWrite(IO, dState);
    }

    if (lState == HIGH && now >= lTimeSet + lOnTime) {
        UnsetLight();
    }
}

void IOtimer::Set(int microseconds) { // Sets jar to on and sets jar timer

    dTimeSet = millis();
    dOnTime = microseconds;
    dState = HIGH;

    pSet(microseconds); // Turns on the pump for the same length of time as the
                        // valve
    digitalWrite(IO, dState);

    SetLight(microseconds); // TODO: check if it works????
}

void IOtimer::SetLight(int microseconds) {
    lTimeSet = millis();
    lOnTime = microseconds;
    lState = HIGH;

    updateLightState(lState);
}

void IOtimer::UnsetLight() {
    lState = LOW;
    updateLightState(lState);
}

void IOtimer::updateLightState(bool state) {
    uint32_t color = state == HIGH ? colors[jarNum % 6] : 0;
    int pixelNum = jarNum * 2;

    strip.setPixelColor(pixelNum, color);
    strip.setPixelColor(pixelNum + 1, color);
}
