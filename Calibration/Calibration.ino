#include <EEPROM.h>
#include <HX711.h>

HX711 scale;
uint8_t scaleDataPin = 6;
uint8_t scaleClockPin = 7;

int eeAddress = 0; //Location we want the data to be put.
float calValue;

void setup()
{
    Serial.begin(115200);
    scale.begin(scaleDataPin, scaleClockPin);
    while (!scale.is_ready());

    Serial.print("UNITS: ");
    Serial.println(scale.get_units(10));

    Serial.println("\nEmpty the scale, press a key to continue");
    while (!Serial.available());
    while (Serial.available()) Serial.read();

    scale.tare();
    Serial.print("UNITS: ");
    Serial.println(scale.get_units(10));

    Serial.println("\nPut 1000 gram in the scale, press a key to continue");
    while (!Serial.available());
    while (Serial.available()) Serial.read();

    scale.calibrate_scale(1000, 5);
    Serial.print("UNITS: ");
    Serial.println(scale.get_units(10));

    calValue = scale.get_scale();
    EEPROM.put(eeAddress, calValue);
    // eeAddress += sizeof(calValue); //Move address to the next byte after float 'f'.

    Serial.println("\nScale is calibrated, press a key to continue");
    while (!Serial.available());
    while (Serial.available()) Serial.read();
}

void loop()
{
    /* Empty loop */
}