#include "IOtimer.h"
#include "common.h"
#include <Arduino.h>

IOtimer::IOtimer(int jarPin, int ledIndex, char jar)
    : IO(jarPin), jarNum(jar), ledIndex(ledIndex), dTimeSet(0), dOnTime(0), dState(LOW), lTimeSet(0),
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

// void IOtimer::SetPump() { // Sets jar to on and sets jar timer
//     pstate = HIGH;
//     digitalWrite(pIO, HIGH);
// }

// void IOtimer::UnsetPump() { // Sets jar to on and sets jar timer
//     pstate = LOW;
//     digitalWrite(pIO, LOW);
// }

void IOtimer::SetLight() {
    // lTimeSet = millis();
    // lOnTime = microseconds;
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

void IOtimer::SetValve() {
    dState = HIGH;
    digitalWrite(IO, dState);
}

void IOtimer::UnsetValve(){
    dState = LOW;
    digitalWrite(IO, dState);
}
