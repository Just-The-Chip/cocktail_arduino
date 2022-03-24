#include <stdint.h>

class BittersArm {
  private:
    // directional pins:
    // C D
    // 0 0 = stopped
    // 1 0 = up
    // 0 0 = down
    uint8_t pinC; // "up" pin called "C" in the documentation
    uint8_t pinD; // "down" pin called "D" in the documenation

    uint8_t bottomLimitPin;
    uint8_t topLimitPin;

  public:
    BittersArm(uint8_t upPin, uint8_t downPin, uint8_t bottomPin, uint8_t topPin);
    void init();
    void lift();
    void lower();
    void stop();

    bool isAtTop();
    bool isAtBottom();
};