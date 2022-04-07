// NOTE: I2C exchanges cannot exceede 32 bytes (32 characters)
// NOTE: IO HIGH is actually relay off.  LOW is on.  All relay IO logic is NOT

#include "IOtimer.h"
#include "BittersArm.h"
#include "BittersHand.h"
#include "common.h"
#include <Adafruit_DotStar.h>
#include <Wire.h>
#include <CRC.h>
#include <HX711.h>
#include <EEPROM.h>
#include <HardwareSerial.h>

#define SLAVE_ADDRESS 0x04

// Load cell stuff
HX711 scale;
uint8_t scaleDataPin = 12;
uint8_t scaleClockPin = 13;

uint8_t bellPin = 0;

uint8_t pIO = 43;               // pin of pump

// handPins(4,5,6,7)
// liftPins(8,9,10,11) 10 = up, 11 = down

uint8_t armBottomLimitPin = 2;
uint8_t armTopLimitPin = 3;

BittersArm bittersArm(10, 11, 2, 3);
BittersHand bittersHand(4, 5, 6, 7);

int bittersPosition = 22;
IOtimer *jars[21];

// Globals:
const int maxBuf = 32;  // Size of serial buffer
unsigned char buf[maxBuf + 1]; // 32 byte serial buffer + termination character
int bufLen = 0;
unsigned long parsedBuf[21][2]; //  stores parsed command data, 21 rows by 2 columns
bool checkBuf = false;
int eepromAddress = 0;

enum command: char {
    pump = 'p',
    show = 's'
};

enum statusCode: char{   
    ack = 0x06, 
    nack = 0x15, 
    reading = 0x05,
    dispensing = 0x07,
    ready = 0x03
};

volatile statusCode status;

Adafruit_DotStar strip = Adafruit_DotStar(90, DOTSTAR_BRG);
uint32_t colors[6];

// SETUP---------------------------------------------------------------------------------------
void setup() {
    Serial.begin(115200);

    Wire.begin(SLAVE_ADDRESS);
    Wire.onReceive(receiveData);
    Wire.onRequest(requestEvent);
    buf[0] = '\0'; // Clears 'buf' by placing termination character at index 0

    pinMode(bellPin, OUTPUT);
    pinMode(pIO, OUTPUT);   // Initializes pump pin
    digitalWrite(pIO, LOW); // Initializes pump pin

    setupBittersDispenser();

    setupStrip();
    setupJars();
    setupScale();
    Serial.println("Rdy to Dispense!");

    status = ready;
}

void setupBittersDispenser() {
    bittersArm.init();
    bittersHand.init();

    // interrupts to stop the motor when it reaches the top or bottom
    // this has to be done here because you can only attach a static function
    attachInterrupt(digitalPinToInterrupt(bittersArm.getBottomLimitInterrupt()), stopArm, FALLING);
    attachInterrupt(digitalPinToInterrupt(bittersArm.getTopLimitInterrupt()), stopArm, FALLING);

    //lower arm if it's not lowered already
    if(!bittersArm.isAtBottom()) {
      bittersArm.lower();
    }
}

// wow I love c++ so much.
void stopArm() {
  bittersArm.stop();
}

void setupStrip() {
    strip.begin();
    strip.show(); // Initialize all pixels to 'off'
    strip.setBrightness(10);
}

void setupJars() {
    IOtimer *jarPointer;
    int startPin = 22;
    for (int i = 0; i < 21; i++) {
        jarPointer = new IOtimer(startPin + i, char(i));
        jars[i] = jarPointer;
    }
}

void setupScale() {
    float calValue;
    
    scale.begin(scaleDataPin, scaleClockPin);
    while (!scale.is_ready());
    EEPROM.get(eepromAddress, calValue);
    scale.set_scale(calValue);
}

void receiveData(int byteCount) {
    int packetLen = 0;
    static int bufIndex = 0;
    static unsigned long lastEvent = millis();

    if (millis() - lastEvent > 1000) {
        buf[packetLen] = '\0';
        bufIndex = 0;
    }

    // If reading first byte of packet
    if (packetLen == 0) {
      packetLen = Wire.read();

      // If packet length is unreasonable
      if (packetLen != (byteCount - 1)) {
          Serial.println("aw heck maxbuff is like too small?");
          packetLen = 0;
          status = nack;
          return;
      }
    }

    status = reading;

    while (Wire.available()) {
        buf[bufIndex] = Wire.read();
        // Serial.print(buf[bufIndex], HEX);
        // Serial.print(" ");
        bufIndex++;
    }

    bufLen = bufIndex;

    // If all bytes of packet were recieved
    if (bufIndex >= (packetLen - 1)) {
        // This adds the string termination character to buf
        buf[packetLen] = '\0';
        packetLen = 0;
        bufIndex = 0;

        checkBuf = true;
    }

    return;
}

void requestEvent() {
    sendStatus();
}

// Forward Declarations
void progress(float currentWeight, float totalWeight, bool changeColor = false);

// LOOP-----------------------------------------------------------------------------------
void loop() {
    // Check buffer for commands, and execute if there are any.
    if (checkBuf) {
        checkBuf = false;
        parseBuf();
    }
    rainbow(25);
}

// Parse single packet out of buf
void parseBuf() {
    static int parsedBufIndex = 0;
    static int numIndeces = 0;

    bool crcGood = checkCRC();
    if (!crcGood) {
        status = nack;
        return;
    }

    // If parsing command (index 0), else if parsing ingredient/jar position
    if (parsedBufIndex == 0) {
        numIndeces = buf[1];
        parsedBuf[parsedBufIndex][0] = buf[0];
        parsedBuf[parsedBufIndex][1] = buf[1];
    } else {
        parsedBuf[parsedBufIndex][0] = buf[0];
        parsedBuf[parsedBufIndex][1] = 0;
        for (int i=1; i<5; i++) {
            parsedBuf[parsedBufIndex][1] |= buf[i];

            if(i < 4) {
              parsedBuf[parsedBufIndex][1] = parsedBuf[parsedBufIndex][1] << 8;
            }    
        }
    }

    buf[0] = '\0';
    status = ack;

    // If full transmission has been parsed
    if (parsedBufIndex >= numIndeces) {
        command recievedCommand = static_cast<command>(parsedBuf[0][0]);

        switch (recievedCommand) {
            case pump:
                doPumpCmd(numIndeces);
                break;
            default:
                Serial.println("Bad command");
                break;
        }
        parsedBufIndex = 0;
        numIndeces = 0;
    } else {
      parsedBufIndex++;
    }
}

float getStableCurrentWeight(float totalWeight) {
    unsigned long startTime = millis();
    float currentWeight = readScale(1);

    // Wait for scale to be stable, then continue
    do {
        // delay N milliseconds, while also updating progress and watching out for sabatage
        unsigned long delayStart = millis();
        while ((millis() - delayStart) < 500) {
            currentWeight = readScale(1);
            progress(currentWeight, totalWeight);
            if (millis() >= startTime + 10000) break;  // timeout
            if (currentWeight < -8000.0) break;  // drink removed
        }
    } while ((abs(readScale(1) - currentWeight)) > 1000);

    return currentWeight;
}

// returns final weight reading to use to check if cup was removed.
float dispenseIngredient(float initialWeight, float targetWeight,float totalWeight, int position) {
    bool isBitters = position == bittersPosition - 1;
    float currentWeight = initialWeight;
    unsigned long startTime = millis();

    if(!isBitters) {
        jars[position] -> SetValve();
    }

    // Check scale until target weight is reached or timeout or drink removed
    while (currentWeight <= targetWeight) {
        progress(currentWeight, totalWeight);

        if (isBitters) {
            bittersHand.shake();
            delay(2000);
        }

        currentWeight = readScale(1);
        // Serial.println(currentWeight);
        if (millis() >= startTime + 40000) break;  // timeout
        if (currentWeight < -8000.0) break;  // drink removed
    }

    // Done with ingredient
    if(!isBitters) {
        jars[position] -> UnsetValve();
    }

    return currentWeight;
}

void doPumpCmd(int numIngredients) {
    status = dispensing;
    progress(0, 0);
    digitalWrite(pIO, true);    // turns on pump

    int bittersListPos = 0; // if this is 0 there is no bitters
    float totalWeight = 0;  // Total drink weight, used for progress animation

    // Get total drink weight from buffer
    for (int i = 1; i < (numIngredients + 1); i++) {  // i=1 because index 0 contains the command

        // "jar" position 22 is exclusively for bitters
        if (parsedBuf[i][0] == bittersPosition) {
            bittersListPos = i;
        }

        totalWeight += parsedBuf[i][1];
    }

    // begin arm lift now because it takes for-ev-errrrrr
    // no worries it's non blocking :)
    if(bittersListPos > 0) {
        bittersArm.lift();
    }

    scale.tare(5);
    float targetWeight = readScale(5); // Get current weight
    bool cupRemoved = false;
    
    // For each Ingredient
    for (int i = 1; i < (numIngredients + 1); i++) {
        if (i == bittersListPos) {
            continue;
        }

        int position = parsedBuf[i][0] - 1;
        float currentWeight = getStableCurrentWeight(totalWeight);

        // Prepare to dispense ingredient
        totalWeight += currentWeight - targetWeight; // Add overflow to total, for progress animation
        targetWeight = readScale(5); // Get current weight
        targetWeight += parsedBuf[i][1]; // Add weight of this ingredient
        if (i>1) progress(currentWeight, totalWeight, true);   // change animation color if not first ingredient, 1 is first ingredient

        float finalIngredientWeight = dispenseIngredient(currentWeight, targetWeight, totalWeight, position);
        cupRemoved = finalIngredientWeight < -8000.0;

        if (cupRemoved) break; // drink removed
    }

    if (bittersListPos > 0 && !cupRemoved) {
        float currentWeight = getStableCurrentWeight(totalWeight);

        // Prepare to dispense ingredient
        totalWeight += currentWeight - targetWeight; // Add overflow to total, for progress animation
        targetWeight = readScale(5); // Get current weight
        targetWeight += parsedBuf[bittersListPos][1]; // Add weight of this ingredient
        if (bittersListPos > 1) progress(currentWeight, totalWeight, true);   // change animation color if not first ingredient, 1 is first ingredient

        dispenseIngredient(currentWeight, targetWeight, totalWeight, bittersPosition - 1);
        bittersArm.lower();
    }

    digitalWrite(pIO, false); // turn off pump
    ringBell();
    status = ready;
}

// Returns true if CRC calculation matches recieved CRC
bool checkCRC() {
    int recievedCRC = buf[bufLen -2];

    int calculatedCRC = crc16((uint8_t *) buf, (bufLen - 2), 0x1021, 0x0000);
    recievedCRC = recievedCRC << 8;
    recievedCRC |= (uint8_t) buf[bufLen - 1];

    return calculatedCRC == recievedCRC;
}

// Sends current status to Pi whenever Pi requests it
void sendStatus() {
    // Serial.println(status);
    Wire.flush();
    Wire.write(status);
    return;
}

// Rainbow animation for LEDs when dispenser is idle
void rainbow(int wait) {
  static long firstPixelHue = 0;
  static unsigned long lastEvent = millis();
  unsigned long now = millis();

  if (now - lastEvent > wait) {

    for(int i=0; i<strip.numPixels(); i++) { 
      int pixelHue = firstPixelHue + (i * 65536L / strip.numPixels());
      strip.setPixelColor(i, strip.gamma32(strip.ColorHSV(pixelHue)));
    }
    strip.show();

    firstPixelHue += 256;

    if (firstPixelHue >=  5*65536) {
      firstPixelHue = 0;
    }

    lastEvent = now;
  }
}

// Rings da bell
void ringBell() {
  // NOTE: We are using delay because it seems we can get away with it
  //       but if issues come up from blocking we will have to refactor. 
  digitalWrite(bellPin, HIGH);
  delay(9);
  digitalWrite(bellPin, LOW);

  delay(150);

  digitalWrite(bellPin, HIGH);
  delay(9);
  digitalWrite(bellPin, LOW);
}

// Displays a progress indicator on the LEDs during dispensing.
void progress(float currentWeight, float totalWeight, bool changeColor = false) {
  static int head = 0;
  static long pixelHue = 0;
  int dunPixels;

  if (changeColor) {
    if ((pixelHue += 8192) >=  5*65536) pixelHue = 0;
    return;
  }

  // Clears the LEDs, resets the animation
  if (currentWeight == 0 && totalWeight == 0) {
      strip.clear();
      strip.show();
      head = 0;
      return;
  }
  
  dunPixels = ceil((currentWeight / totalWeight) * strip.numPixels());
  if (dunPixels < 0) return;    // no reason to continue, and prevents buggy behavior
  if (dunPixels > strip.numPixels()) dunPixels = strip.numPixels();

  while (head < dunPixels) {
    strip.setPixelColor(head, strip.gamma32(strip.ColorHSV(pixelHue)));
    head++;
  }
  strip.show();                     // Refresh strip
}


// Returns scale reading in milligrams
float readScale(uint8_t samples) {
    return scale.get_units(samples) * 1000;
}