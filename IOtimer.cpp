#include "IOtimer.h"
#include "common.h"
#include <Arduino.h>

IOtimer::IOtimer(int pin, int ledIndex, char jar)
    : IO(pin), jarNum(jar), ledIndex(ledIndex), dTimeSet(0), dOnTime(0), dState(LOW), lTimeSet(0),
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

void IOtimer::Set(long microseconds) { // Sets jar to on and sets jar timer

    dTimeSet = millis();
    dOnTime = microseconds; //actually miliseconds????????
    dState = HIGH;

    pSet(microseconds + 8); // Turns on the pump for the same length of time as the
                        // valve
    digitalWrite(IO, dState);

    SetLight(microseconds); // TODO: check if it works????
}

void IOtimer::SetLight(long microseconds) {
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

    strip.setPixelColor(ledIndex, color);
    strip.setPixelColor(ledIndex + 1, color);
}
