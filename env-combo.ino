// Only BME280 uses

#include <stdin.h>
#include "SparkFunMBE280.h"
#include "Wire.h"
#include "SPI.h"

BME280 sensor;

void setup() {
  sensor.settings.commInterface = I2C_MODE;
  sensor.settings.I2CAddress = 0x77;
  sensor.settings.runMode = 3;
  sensor.settings.tStandby = 0;
  sensor.settings.filter = 0;
  sensor.settings.tempOverSample = 1;
  sensor.settings.pressOverSample = 1;
  sensor.settings.humidOverSample = 1;
  delay(10);
  Serial.begin(9600)
  Serial.println("BME280 initialized...");
}
void loop(){
  Serial.println("Pressure: " + String(mySensor.readFloatPressure()) + "kPa , Temp: " + String(mySensor.readTempC()) + "C , Alt: " + String(mySensor.readFloatAltitudeMeters()) + "m , Hum: " + String(mySensor.readFloatHumidity()) + "%");
  delay(1000);
}
