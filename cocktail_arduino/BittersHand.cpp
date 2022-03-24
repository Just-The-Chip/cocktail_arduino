#include <stdint.h>
#include <Arduino.h>
#include <Stepper.h>
#include "BittersHand.h"

BittersHand::BittersHand(uint8_t pin1, uint8_t pin2, uint8_t pin3, uint8_t pin4) {
    pin1 = pin1;
    pin2 = pin2;
    pin3 = pin3;
    pin4 = pin4;

    stepper = new Stepper(stepsPerRevolution, pin1, pin2, pin3, pin4);
}

void BittersHand::init() {
    // I guess this also sets pinMode?
    stepper.setSpeed(20);
}

void BittersHand::shake() {
    step(shakeRotation);
    step(shakeRotation * -1);
    
    // hold power on for a short time to stop momentum 
    sleep(holdTime);

    powerOff();
}

void BittersHand::step(int degrees) {
    const double stepDegrees = 360.0 / stepsPerRevolution;
    double steps = degrees / stepDegrees;
    stepper.step((int) steps);
}

// turns all pins off so the motor doesn't get hot when not in use
void BittersHand::powerOff() {
    digitalWrite(pin1, LOW);
    digitalWrite(pin2, LOW);
    digitalWrite(pin3, LOW);
    digitalWrite(pin4, LOW);
}