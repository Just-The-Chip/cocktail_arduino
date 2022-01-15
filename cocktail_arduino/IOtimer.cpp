#include "IOtimer.h"
#include "common.h"
#include <Arduino.h>

IOtimer::IOtimer(int jarPin, char jar)
    : IO(jarPin), jarNum(jar) {
    
    pinMode(IO, OUTPUT);
    // Initialize each valve IO High to set all relays Low.
    digitalWrite(IO, LOW);
}

void IOtimer::SetValve() {
    digitalWrite(IO, HIGH);
}

void IOtimer::UnsetValve(){
    digitalWrite(IO, LOW);
}
