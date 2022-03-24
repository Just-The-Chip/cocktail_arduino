#include <stdint.h>
#include <Arduino.h>
#include "BittersArm.h"

BittersArm::BittersArm(uint8_t upPin, uint8_t downPin, uint8_t bottomPin, uint8_t topPin) {
    pinC = upPin;
    pinD = downPin;
    bottomLimitPin = bottomPin;
    topLimitPin = topPin;
}

void BittersArm::init() {
    pinMode(pinC, OUTPUT);
    pinMode(pinD, OUTPUT);

    pinMode(bottomLimitPin, INPUT);
    pinMode(topLimitPin, INPUT);

    // ensure pins are in the stopped state
    stop();

    // interrupts to stop the motor when it reaches the top or bottom
    attachInterrupt(digitalPinToInterrupt(bottomLimitPin), stop, RISING);
    attachInterrupt(digitalPinToInterrupt(topLimitPin), stop, RISING);
}

void BittersArm::lift() {
    digitalWrite(pinC, HIGH);
    digitalWrite(pinD, LOW);
}

void BittersArm::lower() {
    digitalWrite(pinC, LOW);
    digitalWrite(pinD, HIGH);
}

void BittersArm::stop() {
    digitalWrite(pinC, LOW);
    digitalWrite(pinD, LOW);
}

bool BittersArm::isAtBottom() {
    return digitalRead(bottomLimitPin);
}

bool BittersArm::isAtTop() {
    return digitalRead(topLimitPin);
}