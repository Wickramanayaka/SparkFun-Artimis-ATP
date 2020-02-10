/* Copyright 2019 The TensorFlow Authors. All Rights Reserved.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
==============================================================================*/

#include <TensorFlowLite.h>
#include "main_functions.h"
#include "constants.h"
#include "output_handler.h"
#include "sine_model_data.h"
#include "tensorflow/lite/experimental/micro/kernels/all_ops_resolver.h"
#include "tensorflow/lite/experimental/micro/micro_error_reporter.h"
#include "tensorflow/lite/experimental/micro/micro_interpreter.h"
#include "tensorflow/lite/schema/schema_generated.h"
#include "tensorflow/lite/version.h"

namespace {
tflite::ErrorReporter* error_reporter = nullptr;
const tflite::Model* model = nullptr;
tflite::MicroInterpreter* interpreter = nullptr;
TfLiteTensor* input = nullptr;
TfLiteTensor* output = nullptr;
int inference_count = 0;
constexpr int kTensorArenaSize = 2 * 1024;
uint8_t tensor_arena[kTensorArenaSize];
}

#include <stdint.h>
#include "SparkFunBME280.h"
#include <SoftwareSerial.h>
#include "Wire.h"
#include "SPI.h"
#include "SparkFun_VCNL4040_Arduino_Library.h"
#include <SFE_MicroOLED.h>

#define PIN_RESET 9  
#define DC_JUMPER 1

BME280 mySensor;
VCNL4040 proximitySensor;
MicroOLED oled(PIN_RESET, DC_JUMPER);
SoftwareSerial esp8266(9,10);

String proxValue;
String temp;
String humidity;
String bp;
float rain;
String alt;

void setup() {
  // ML setup
  static tflite::MicroErrorReporter micro_error_reporter;
  error_reporter = &micro_error_reporter;
  model = tflite::GetModel(g_sine_model_data);
  if (model->version() != TFLITE_SCHEMA_VERSION) {
    error_reporter->Report(
        "Model provided is schema version %d not equal "
        "to supported version %d.",
        model->version(), TFLITE_SCHEMA_VERSION);
    return;
  }
  static tflite::ops::micro::AllOpsResolver resolver;
  static tflite::MicroInterpreter static_interpreter(
      model, resolver, tensor_arena, kTensorArenaSize, error_reporter);
  interpreter = &static_interpreter;
  TfLiteStatus allocate_status = interpreter->AllocateTensors();
  if (allocate_status != kTfLiteOk) {
    error_reporter->Report("AllocateTensors() failed");
    return;
  }
  input = interpreter->input(0);
  output = interpreter->output(0);
  inference_count = 0;

  // Sensor setup
  mySensor.settings.commInterface = I2C_MODE;
  mySensor.settings.I2CAddress = 0x77;
  mySensor.settings.runMode = 3;
  mySensor.settings.tStandby = 0;
  mySensor.settings.filter = 0;
  mySensor.settings.tempOverSample = 1;
  mySensor.settings.pressOverSample = 1;
  mySensor.settings.humidOverSample = 1;
  Serial.begin(9600);
  esp8266.begin(9600);
  Serial.print("Starting BME280... result of .begin(): 0x");
  Serial.println(mySensor.begin(), HEX);
  Wire.begin();
  if (proximitySensor.begin() == false)
  {
    Serial.println("Device not found. Please check wiring.");
    while (1);
  }
  // Initialize display
  oled.begin(); 
  oled.clear(ALL);
  oled.display(); 
  delay(1000);  
  oled.clear(PAGE);
}

void loop() {
  // Clear display
  oled.clear(PAGE);
  oled.setFontType(0.25);
  oled.setCursor(0,0);

  proxValue = String(proximitySensor.getProximity()); 
  temp = String(mySensor.readTempC());
  humidity = String(mySensor.readFloatHumidity());
  bp = String(round(mySensor.readFloatPressure()));
  alt = String(mySensor.readFloatAltitudeMeters());

  // Do the inference
  rain = inference(temp.toInt(), humidity.toInt(), bp.toInt());
  
  // Display sensor data
  oled.print(temp + "C");
  oled.setCursor(0,10);
  oled.print(humidity + "%");
  oled.setCursor(0,20);
  oled.print(bp + "kPa");
  //oled.setCursor(0,30);
  //oled.print(alt + "m");
  //oled.setCursor(0,30);
  //oled.print(proxValue);
  oled.setCursor(0,30);
  oled.print("Rain prob.: ");
  oled.setCursor(0,40);
  oled.print(String(rain) + "%");
  oled.display();
  // Send values to ESP8266 IoT gateway
  esp8266.print(proxValue);
  esp8266.print(",");
  esp8266.print(mySensor.readTempC());
  esp8266.print(",");
  esp8266.print(mySensor.readFloatHumidity());
  esp8266.print(",");
  esp8266.print(round(mySensor.readFloatPressure()));
  esp8266.print("\r\n");
  delay(1000);
}

float inference(int x, int y, int z){
  input->data.f[0] = x;
  input->data.f[1] = y;
  input->data.f[2] = z;
  TfLiteStatus invoke_status = interpreter->Invoke();
  if (invoke_status != kTfLiteOk) {
    error_reporter->Report("Invoke failed...");
    return 0;
  }
  float y_val = output->data.f[0];
  return y_val/100;
}
