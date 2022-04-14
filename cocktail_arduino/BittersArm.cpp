#include <stdint.h>
#include <Arduino.h>
#include "BittersArm.h"

BittersArm::BittersArm(uint8_t upPin, uint8_t downPin, uint8_t bottomPin, uint8_t topPin) {
    pinC = upPin;
    pinD = downPin;
    bottomLimitPin = bottomPin;
    topLimitPin = topPin;
}

// NOTE: This function does NOT setup the stop inturrupts. That MUST be done outside of this class. 
void BittersArm::init() {
    pinMode(pinC, OUTPUT);
    pinMode(pinD, OUTPUT);

    pinMode(bottomLimitPin, INPUT_PULLUP);
    pinMode(topLimitPin, INPUT_PULLUP);

    // ensure pins are in the stopped state
    stop();
}

void BittersArm::lift() {
    if (isAtTop()) return;

    digitalWrite(pinC, HIGH);
    digitalWrite(pinD, LOW);
}

void BittersArm::lower() {
    if (isAtBottom()) return;

    digitalWrite(pinC, LOW);
    digitalWrite(pinD, HIGH);
}

void BittersArm::stop() {
    digitalWrite(pinC, LOW);
    digitalWrite(pinD, LOW);
}

bool BittersArm::isAtBottom() {
    return !digitalRead(bottomLimitPin);
}

bool BittersArm::isAtTop() {
    return !digitalRead(topLimitPin);
}

uint8_t BittersArm::getBottomLimitInterrupt() {
  return digitalPinToInterrupt(bottomLimitPin);
}

uint8_t BittersArm::getTopLimitInterrupt() {
  return digitalPinToInterrupt(topLimitPin);
}