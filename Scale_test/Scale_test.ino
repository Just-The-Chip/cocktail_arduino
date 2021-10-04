#include <EEPROM.h>
#include <HX711.h>

HX711 scale;
uint8_t scaleDataPin = 6;
uint8_t scaleClockPin = 7;
int weight = 100; // Weight of the calibration weight

int eeAddress = 0; //Location we want the data to be put.
float calValue;

void setup()
{
    Serial.begin(115200);
    scale.begin(scaleDataPin, scaleClockPin);
    while (!scale.is_ready());

    scale.tare();
    EEPROM.get(eeAddress, calValue);
    scale.set_scale(calValue);
}

void loop()
{
    Serial.println(scale.get_units(10));

}
