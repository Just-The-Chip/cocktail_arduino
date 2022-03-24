#include <stdint.h>
#include <Stepper.h>

class BittersHand {
  private:
    const int stepsPerRevolution = 200*14;
    const int shakeRotation = 130;
    const unsigned int holdTime = 250;

    Stepper stepper; 
    uint8_t pin1;
    uint8_t pin2;
    uint8_t pin3;
    uint8_t pin4;

    void step(int degrees);
    void powerOff();

  public:
    BittersHand(uint8_t pin1, uint8_t pin2, uint8_t pin3, uint8_t pin4);
    void init();
    void shake();
};