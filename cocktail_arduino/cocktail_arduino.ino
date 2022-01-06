// NOTE: I2C exchanges cannot exceede 32 bytes (32 characters)
// NOTE: IO HIGH is actually relay off.  LOW is on.  All relay IO logic is NOT

// High level Dispense sequence:
// Set pump and (n)air valve on for (n)miliseconds
// Repeat for each  individual air valve (pi will send a sequence of commands
// with appropriate delay between them)

// Commands:
// pump:'jar':'mS'  This will pump 'jar' for 'mS' time

#include "IOtimer.h"
#include "common.h"
#include <Adafruit_DotStar.h>
#include <Wire.h>
#include <CRC.h>
#include <HX711.h>
#include <EEPROM.h>
#include <HardwareSerial.h>
// #define LIGHTDATA 12
// #define LIGHTCLOCK 11

#define SLAVE_ADDRESS 0x04

// Load cell stuff
HX711 scale;
uint8_t scaleDataPin = 6;
uint8_t scaleClockPin = 7;

IOtimer *jars[21];
int colorIndicies[21] = {
    0,  //0
    6,  //1
    11, //2
    16, //3

    18, //4
    23, //5
    29, //6
    34, //7

    36, //8
    41, //9
    47, //10
    52, //11

    54, //12
    59, //13
    65, //14
    70, //15

    72, //16
    76, //17
    80, //18
    85, //19
    88  //20
};

// Globals:
const int maxBuf = 32;  // Size of serial buffer
unsigned char buf[maxBuf + 1]; // 32 byte serial buffer + termination character
int bufLen = 0;
unsigned long now = millis();
unsigned long parsedBuf[21][2]; //  stores parsed command data, 21 rows by 2 columns
bool checkBuf = false;
int eepromAddress = 0;
// volatile bool ack = false;

// Variables for pump operation
int pIO = 43;               // pin of pump
long ptimeSet = 0; // mS time that pump was set to on
long ponTime = 0;  // mS time for pump to stay on
bool pstate = LOW;          // Is HIGH when pump is active

enum command: char{
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
    // Wire.onRequest(sendData);
    buf[0] = '\0'; // Clears 'buf' by placing termination character at index 0

    pinMode(pIO, OUTPUT);   // Initializes pump pin
    digitalWrite(pIO, LOW); // Initializes pump pin

    setupStrip();
    setupJars();
    setupScale();
    Serial.println("Rdy to Dispense!");

    status = ready;
}

void setupStrip() {
    colors[0] = strip.Color(255, 0, 32);  // pink
    colors[1] = strip.Color(255, 32, 0);  // orange
    colors[2] = strip.Color(224, 255, 0); // green
    colors[3] = strip.Color(0, 255, 128); // bluegreen
    colors[4] = strip.Color(0, 0, 255);   // blue
    colors[5] = strip.Color(255, 0, 255); // magenta

    strip.begin();
    strip.show(); // Initialize all pixels to 'off'
}

void setupJars() {
    IOtimer *jarPointer;
    int startPin = 22;
    // TODO: Load scale calibration
    for (int i = 0; i < 21; i++) {
        jarPointer = new IOtimer(startPin + i, colorIndicies[i], char(i));
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

    if (millis - lastEvent > 1000) {
        buf[packetLen] = '\0';
        bufIndex = 0;
    }
    
    //  Serial.print("B");
    //  Serial.println(byteCount);

    // If reading first byte of packet
    if (packetLen == 0) {
      packetLen = Wire.read();

      // Serial.print("P");
      // Serial.println(packetLen);

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

// LOOP-----------------------------------------------------------------------------------
void loop() {
    // Check buffer for commands, and execute if there are any.
    if (checkBuf) {
        checkBuf = false;
        parseBuf();
    }
}

// Parse single packet out of buf
void parseBuf() {
    static int parsedBufIndex = 0;
    static int numIndeces = 0;

    bool crcGood = checkCRC();
    if (!crcGood) {
        // Serial.println("!C");
        // Serial.println("---");
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

    buf[0] = "\0";
    status = ack;

    // If full transmission has been parsed
    if (parsedBufIndex >= numIndeces) {
        command recievedCommand = parsedBuf[0][0];

        switch (recievedCommand) {
            case pump:
                doPumpCmd(numIndeces);
                break;
            case show: // TODO: remove
                doShowCmd(numIndeces);
                break;
            default:
                // Serial.println("Bad command");
                break;
        }
        parsedBufIndex = 0;
        numIndeces = 0;
    } else {
      parsedBufIndex++;
    }

    // Serial.println("-----");
}

void doPumpCmd(int numIngredients) {
    status = dispensing;

    strip.clear();              // turn off all LEDs
    digitalWrite(pIO, true);    // turns on pump
    // TODO: Add check to make sure cup is placed
    scale.tare(5);
    float drinkWeight = scale.get_units(5) * 1000; // Get current weight
    
    // For each Ingredient
    for (int i = 1; i < (numIngredients + 1); i++) {  // i=1 because index 0 contains the command
        int position = parsedBuf[i][0] - 1;

        // Check for stability, then tare scale
        float lastReading = scale.get_units(1);
        while ((abs(scale.get_units(1) - lastReading)) > 1.0) {
            delay(500);
            lastReading = scale.get_units(1);
            Serial.println(lastReading);
        }

        drinkWeight = scale.get_units(5) * 1000; // Get current weight
        drinkWeight += parsedBuf[i][1]; // Add weight of this ingredient
        unsigned long pumpStart = millis();

        jars[position] -> SetLight();
        jars[position] -> SetValve();
        // Check scale until target weight is reached or timeout
        float currentWeight = scale.get_units(1) * 1000;
        while (currentWeight <= drinkWeight) {
            currentWeight = scale.get_units(1) * 1000;
            Serial.println(currentWeight);
            if (millis() >= pumpStart + 40000) {
                break;
            }
            if (currentWeight < -8000.0) {
              break;
            }
        }
        
        jars[position] -> UnsetValve();
        jars[position] -> UnsetLight();

        if (currentWeight < -8000.0) {
              break;
        }

        unsigned long currentTime = millis();
    }

    digitalWrite(pIO, false); // turn off pump
    status = ready;
}

void doShowCmd(int numPositions) {
    strip.clear();              // turn off all LEDs
    
    // For each position
    for (int i = 1; i < numPositions; i++) {  // i=1 because index 0 contains the command
        int position = parsedBuf[i][0];
        jars[position] -> SetLight();
    }
}

// This update routine is specifically for the pump
void pUpdate() {
    // IF jar is on and onTime has elapsed, turn pump off.
    if (pstate == HIGH &&  now >= ptimeSet + ponTime) {
        digitalWrite(pIO, LOW);
    }
}

// This Set routine is specifically for the pump
void pSet(long microseconds) {

    ptimeSet = millis();
    ponTime = microseconds; //actually miliseconds?
    pstate = HIGH;

    digitalWrite(pIO, HIGH);
    // Serial.print("pSet works");
}

bool checkCRC() {
    int recievedCRC = buf[bufLen -2];

    int calculatedCRC = crc16((uint8_t *) buf, (bufLen - 2), 0x1021, 0x0000);
    recievedCRC = recievedCRC << 8;
    recievedCRC |= (uint8_t) buf[bufLen - 1];

    return calculatedCRC == recievedCRC;
}

void sendStatus() {
    // TODO: implement nack
    Serial.println(status);
    Wire.flush();
    Wire.write(status);
    return;
}
