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

// #define LIGHTDATA 12
// #define LIGHTCLOCK 11

#define SLAVE_ADDRESS 0x04

IOtimer *jars[21];
int colorIndicies[21] = {
    0,  //0
    6,  //1
    12, //2
    17, //3
    19, //4
    24, //5
    30, //6
    35, //7
    37, //8
    42, //9
    48, //10
    53, //11
    55, //12
    60, //13
    66, //14
    71, //15
    73, //16
    77, //17
    81, //18
    86, //19
    89  //20
};

// Globals:
char buf[33]; // 32 byte serial buffer + termination character
unsigned long now = millis();
char pumc = 40;

// Variables for pump operation
int pIO = 43;               // pin of pump
long ptimeSet = 0; // mS time that pump was set to on
long ponTime = 0;  // mS time for pump to stay on
bool pstate = LOW;          // Is HIGH when pump is active

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
    for (int i = 0; i < 21; i++) {
        jarPointer = new IOtimer(startPin + i, colorIndicies[i], char(i));
        jars[i] = jarPointer;
    }
}

void receiveData(int byteCount) {
    // Number of bytes being sent by master
    Serial.println("receiving data?");
    int len = Wire.read();
    Serial.println(len);

    // For number of bytes sent, load buf with each byte
    for (int i = 0; i < len; i++) {
        buf[i] = Wire.read();
    }
    // This adds the string termination character to buf
    buf[len] = '\0';
    Serial.println(buf);
}

// LOOP-----------------------------------------------------------------------------------
void loop() {
    // Check buffer for commands, and execute if there are any.
    checkbuf();

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

void checkbuf() {
    // This pointer is used by the strtok commands below
    char *point = strtok(buf, ":");

    //get remainder of string
    char *params = strtok(NULL, "\0");
    if (strcmp(point, "pump") == 0) {
        doPumpCmd(params);
    } else if (strcmp(point, "show") == 0) {
        doShowCmd(params);
    }

    // Clears 'buf' by placing termination character at index 0
    buf[0] = '\0';
}

void doPumpCmd(char *params) {
    Serial.println(buf);
    // // This pointer is used by the strtok commands below
    // char *point = strtok(buf, ":");
    Serial.println(params);

    char *point = strtok(params, ":");
    int jarNum = atoi(point);

    point = strtok(NULL, ":");
    long int mS = strtol(point,NULL,10);

    Serial.print("Jar num: ");
    Serial.println(jarNum);
    Serial.print("ms: ");
    Serial.println(mS);

    if (jarNum > 0 && jarNum <= 21) {
        (jars[jarNum - 1])->Set(mS);
    } else {
        Serial.println("Unhandled jar# selected for pump");
    }
}

void doShowCmd(char *params) {
    Serial.println("BEGIN show:");
    // turn off all jar lights that are currently on
    for (int i = 0; i < 21; i++) {
        (jars[i])->UnsetLight();
    }

    //iterate through comma separated list of jars
    char *point = strtok(params, ",");

    while (point != NULL) {
        Serial.print(point);
        Serial.print(", ");

        int jarNum = atoi(point);

        if (jarNum > 0 && jarNum <= 21) {
            // light it up for 30s
            (jars[jarNum - 1])->SetLight(30 * 1000);
        } else {
            Serial.println("Unhandled jar# selected for show");
        }

        point = strtok(NULL, ",");
    }

    Serial.println("\nEND show");
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
