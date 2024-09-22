
#include <Arduino.h>
#include "CRSFforArduino.hpp"

/**
 CRSF Sensor - Temperature / Thermistor
*/

// CRSF setup
CRSFforArduino *crsf;

const int ledPin = 25;  // Onboard LED pin for RP2040
const unsigned long UPDATE_INTERVAL = 50; // 50ms



// Thermistor -------------------------------
const int thermistorPin = A0;
const float resistorValue = 9300.0;  // Value of the voltage divider resistor in ohms

// Steinhart-Hart coefficients for thermistor
const float A = 0.001129148;
const float B = 0.000234125;
const float C = 0.0000000876741;

// Reads thermistor in degrees Celcius
float readThermistor(int inputPin) {
  int rawADC = analogRead(inputPin);
 
  // Calculate resistance of thermistor
  float voltage = rawADC * (3.3 / 1023.0);  // Assuming 3.3V reference voltage
  float resistance = resistorValue * (3.3 / voltage - 1.0);
 
  // Steinhart-Hart equation
  float logR = log(resistance);
  float temperatureK = 1.0 / (A + B * logR + C * logR * logR * logR);

  return (temperatureK - 273.15);
}
// --------------------------------- Thermistor



void setup() {
    Serial1.begin(420000);
   
    // Initialize CRSF input on Serial1
    crsf = new CRSFforArduino(&Serial1);
    crsf->begin();

    // Set up thermistor pin
    pinMode(thermistorPin, INPUT);

    // Set up LED
    pinMode(ledPin, OUTPUT);
    digitalWrite(ledPin, HIGH);

    // Update CRSF to kick it off...
    crsf->update();
}



void loop() {

  // Get the data from sensors/thermistors
  float temperatureC = readThermistor(thermistorPin);
  float temperatureF = (temperatureC * 9.0 / 5.0) + 32.0;


  // --------------------

  // Vario --
  // NOTE: this has an "Alt", and so does GPS below (ie: two altitude values are served and can be used individually)
  crsf->telemetryWriteBaroAltitude(
    12345, // altitude       - Alt  - [ 12345 shows in as EdgeTx as: 1234.5 - multiply by 10 ]
    6780   // vertical speed - VSpd - [ 6780 shows in as EdgeTx as: 67.8]
  );

  // GPS --
  // NOTE: GPS has an "Alt", and so does BaroAltitude above (ie: two altitude values are served and can be used individually)
  crsf->telemetryWriteGPS( 0.0, 0.0, // latitude/longitude - not useful for hijacking purposes
    12345.0,                // altitude  - Alt  - [ "12345" shows in as EdgeTx as 123 - multiply by 100 ]
    calculateGPSSpeed(678), // speed     - GSpd - you need "calculateTelemetrySpeed" for this to read correctly
    259,                    // heading   - Hdg  - low numbers, you're subject to being within compass direction. "259" is fine, so good for temperatures
    100                     // GPS sats  - Sats - number of GPS satellites [ reads as sent, but only low numbers (8 bit number) ]
  );

  // Battery --
  crsf->telemetryWriteBattery(
    12345, // voltage       - RxBat - [ "12345" shows in as EdgeTx "123.5" - multiply by 100 ]
    12345, // current       - Curr  - [ "12345" shows in as EdgeTx "123.4" - multiply by 100 ]
    12345, // capacity left - Capa  - [ no change to EdgeTx ]
    123    // capacity pct  - Bat%  - [ no change, but small 8 bit number, "123" is "123%" ]
  );

  // Attitude --
  // These values reach EdgeTx as a decimal (2.343), but you can multiply by 100 to shift the decimal two places, etc
  // (although the "100" needs to come from another line in EdgeTx's telemetry, a "calculated" sensor in EdgeTx)
  //
  // Need to use "radiansToCrsfAngle()" to get the value expected
  crsf->telemetryWriteAttitude(
    radiansToCrsfAngle(2.345f), // roll  - Roll - [ "2.345f" shows in as EdgeTx "2.34" ]
    radiansToCrsfAngle(1.234f), // pitch - Ptch - [ "1.231f" shows in as EdgeTx "-1.23" (no idea why it went negative, something to do with the radians) ]
    radiansToCrsfAngle(0.69f)   // yaw   - Yaw  - [ "2.343f" shows in as EdgeTx "0.69" ]
  );

  // --------------------


  // Update CRSF (send/receive)
  crsf->update();

  // wait a little, then go again...
  delay(UPDATE_INTERVAL);
}




// CRSF library being used does something strange to the speed,
// this takes the value you want to see and deals with it, so that what you see in EdgeTx is what you expect
float calculateGPSSpeed(float value) {
    float speed = ((value * 1000.0f) - 50.0f) / 36.0f;
    return max(speed, 0.0f); // Ensure non-negative speed
}

// It arrives in EdgeTx in radians, but the library wants "decidegrees" for roll/pitch/yaw,
// so this takes a value you can read/understand, and is what will be seen in EdgeTx
int16_t radiansToCrsfAngle(float radians) {
    // Convert radians to decidegrees and ensure it's within the -1800 to 1800 range
    int16_t decidegrees = static_cast<int16_t>(radians * 1800.0f / PI);
    return constrain(decidegrees, -1800, 1800);
}





