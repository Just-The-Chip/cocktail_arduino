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
char buf[maxBuf + 1]; // 32 byte serial buffer + termination character
unsigned long now = millis();
// char pumc = 40; <-- Not used?
unsigned long parsedBuf[21][2]; //  stores parsed command data, 21 rows by 2 columns
bool checkBuf = false;

// Variables for pump operation
int pIO = 43;               // pin of pump
long ptimeSet = 0; // mS time that pump was set to on
long ponTime = 0;  // mS time for pump to stay on
bool pstate = LOW;          // Is HIGH when pump is active

enum command: char{
    pump = 'p',
    show = 's'
}

Adafruit_DotStar strip = Adafruit_DotStar(90, DOTSTAR_BRG);
uint32_t colors[6];

// SETUP---------------------------------------------------------------------------------------
void setup() {
    Serial.begin(115200);

    Wire.begin(SLAVE_ADDRESS);
    Wire.onReceive(receiveData);
    // Wire.onRequest(sendData);
    buf[0] = '\0'; // Clears 'buf' by placing termination character at index 0

    pinMode(pIO, OUTPUT);   // Initializes pump pin
    digitalWrite(pIO, LOW); // Initializes pump pin

    setupStrip();
    setupJars();
    scale.begin(scaleDataPin, scaleClockPin);
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
        jarPointer = new IOtimer(dataPin, clockPin, startPin + i, colorIndicies[i], char(i));
        jars[i] = jarPointer;
    }
}

void receiveData(int byteCount) {
    static int packetLen = 0;
    static int bufIndex = 0;
    
    // Number of bytes being sent by master
    Serial.print("bytes incomming: ");
    Serial.println(byteCount);

    // If reading first byte of packet
    if (packetLen == 0) {
        packetLen = Wire.read();
        // If packet length is unreasonable
        if (packetLen > maxBuf) {
            packetLen = 0;
            nack();
            return;
        }
    } else {
        while (Wire.available()) {
            buf[bufIndex] = Wire.read();
            bufIndex++;
        }
    }
    
    // If all bytes of packet were recieved
    if (bufIndex >= (packetLen - 1)) {
        // This adds the string termination character to buf
        buf[packetLen] = '\0';
        packetLen = 0;
        bufIndex = 0;
        Serial.println(buf);
        checkBuf = true;
    }

    return;
}

// LOOP-----------------------------------------------------------------------------------
void loop() {
    // Check buffer for commands, and execute if there are any.
    if (checkBuf) {
        checkBuf = false;
        parseBuf();
    }
    now = millis();

    // Update all jars to see if the valve or lights need to be turned off
    for (int i = 0; i < 21; i++) {
        (jars[i])->Update();
    }

    // Update to see if pump needs to be turned off
    pUpdate();

    // commit LED updates made by jars
    strip.show();
}

// void checkbuf() {
//     // This pointer is used by the strtok commands below
//     char *point = strtok(buf, ":");

//     //get remainder of string
//     char *params = strtok(NULL, "\0");
//     if (strcmp(point, "pump") == 0) {
//         doPumpCmd(params);
//     } else if (strcmp(point, "show") == 0) {
//         doShowCmd(params);
//     }

//     // Clears 'buf' by placing termination character at index 0
//     buf[0] = '\0';
// }

// Parse single packet out of buf
void parseBuf() {
    static int parsedBufIndex = 0;
    static int numIndeces = 0;

    bool crcGood = checkCRC();
    if (!crcGood) {
        nack();
        return;
    }

    // If parsing command
    if (parsedBufIndex == 0) {
        numIndeces = buf[1];
        parsedBuf[parsedBufIndex][0] = buf[0];
        parsedBuf[parsedBufIndex][1] = buf[1];
    } else {
        parsedBuf[parsedBufIndex][0] = buf[0];
        parsedBufIndex[parsedBufIndex][1] = 0;
        for (int i=1; i<5; i++) {
            parsedBufIndex[parsedBufIndex][1] |= buf[i];
            parsedBufIndex[parsedBufIndex][1] = parsedBufIndex[parsedBufIndex][1] << 8;
        }
    }


    buf[0] = "\0";
    ack();

    // If full transmission has been parsed
    if (parsedBufIndex >= numIndeces) {
        command = parsedBuf[0][0];
        switch (command) {
            case pump:
                doPumpCmd();
                break;
            case show:
                doShowCmd();
                break;
            default:
                Serial.println("Bad command");
        }
        parsedBufIndex = 0;
        numIndeces = 0;
    }
}

// void doPumpCmd(char *params) {
//     Serial.println(buf);
//     // // This pointer is used by the strtok commands below
//     // char *point = strtok(buf, ":");
//     Serial.println(params);

//     char *point = strtok(params, ":");
//     int jarNum = atoi(point);

//     point = strtok(NULL, ":");
//     long int mS = strtol(point,NULL,10);

//     Serial.print("Jar num: ");
//     Serial.println(jarNum);
//     Serial.print("ms: ");
//     Serial.println(mS);

//     if (jarNum > 0 && jarNum <= 21) {
//         (jars[jarNum - 1])->Set(mS);
//     } else {
//         Serial.println("Unhandled jar# selected for pump");
//     }
// }

void doPumpCmd(int numIngredients) {
    // LEFT OFF hERE
    allLightsOff();
    digitalWrite(pIO, true); // turns on pump
    
    // For number of Ingredients
    for (int i = 1; i < numIngredients; i++) {
        IOtimer[parsedBuf[i][0]] <- SetLight();  // TODO: fix setlight to only turn on light
        scale.tare(5);
        unsigned long startPump = millis();

        unsigned long ingredientWeight = parsedBuf[i][1];
        while (scale.get_units(2) <= ingredientWeight) {
            if (millis() >= startPump + 120000) {
                break;
            }
        }
        IOtimer[parsedBuf[i][0]] <- UnsetLight();
    }

    digitalWrite(pIO, false);
}

// void your mom
// void doShowCmd(char *params) {
//     Serial.println("BEGIN show:");
//     // turn off all jar lights that are currently on
//     for (int i = 0; i < 21; i++) {
//         (jars[i])->UnsetLight();
//     }

//     //iterate through comma separated list of jars
//     char *point = strtok(params, ",");

//     while (point != NULL) {
//         Serial.print(point);
//         Serial.print(", ");

//         int jarNum = atoi(point);

//         if (jarNum > 0 && jarNum <= 21) {
//             // light it up for 30s
//             (jars[jarNum - 1])->SetLight(30 * 1000);
//         } else {
//             Serial.println("Unhandled jar# selected for show");
//         }

//         point = strtok(NULL, ",");
//     }

//     Serial.println("\nEND show");
// }

void doShowCmd() {
    return;
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
    Serial.print("pSet works");
}

bool checkCRC() {
    int recievedCRC = buf[2];
    int calculatedCRC = crc16_CCITT((uint8_t *) buf, (strlen(buf) - 2));  // generate CRC of buf, excluding recieved CRC
    recievedCRC = recievedCRC << 8;
    recievedCRC |= buf[3];

    return calculatedCRC == recievedCRC;
}

void nack() {
    // TODO: implement nack
    Wire.flush();
    return;
}

void ack() {
    // TODO: implement ack
    return;
}

void allLightsOff() {
    // TODO: Implement all lights off
    return;
}