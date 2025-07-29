#include <Arduino.h>
#include <TXOnlySerial.h>
#include <Wire.h>
#include "magnetometer.h"
// #include <LIS2MDLSensor.h>

#define SERIAL_DEBUG 1

#define DEBUG_OUTPUT_PIN    PIN_A7
#define CDS_INPUT_PIN       PIN_A3
#define METER_OUTPUT_PIN    PIN_A6
#define METER_OUTPUT_MIN_DAC    44
#define METER_OUTPUT_MAX_DAC    164

// Source of coefficients: https://docs.google.com/spreadsheets/d/10hfEoG8dDYRyn8PrnT6vj_a-6OrD_DNj-Ta4taBO-kc/edit?usp=sharing
const float lightInputPoly[] = {3.6, 0.0417, -9.43e-5, 1.04e-7, -3.68e-11};
const float shutterIsoPoly[] = {1.0, 0.05};
const float meterOutputPoly[] = {103.0, 30.4};

#define GET_POLY_ORDER(x) (sizeof(x) / sizeof(x[0]) - 1)

// Debug
#if SERIAL_DEBUG
TXOnlySerial mySerial(DEBUG_OUTPUT_PIN);
#endif

void debugPrintStr(const char *str) {
#if SERIAL_DEBUG && 1
    mySerial.print(str);
#endif
}

void debugPrintlnStr(const char *str) {
#if SERIAL_DEBUG && 1
    mySerial.println(str);
#endif
}

void debugPrintInt(int n) {
#if SERIAL_DEBUG && 1
    mySerial.print(n);
#endif
}

void debugPrintlnInt(int n) {
#if SERIAL_DEBUG && 1
    mySerial.println(n);
#endif
}

void debugPrintlnF(float n) {
#if SERIAL_DEBUG
#if 1
    // Cannot use standard print with floating-point libraries due to limited flash space,
    // so this implements a simple float to string conversion with 1 decimal place.
    if (n < 0.0) {
        debugPrintStr("-");
        n = -n; // Make it positive for processing
    }
    int integerPart = (int)n;   // truncate
    int decimalPart = (int)((n - (float)integerPart) * 10.0); // Get the first decimal place
    // int integerPart = v1;
    // int decimalPart = v2;
    debugPrintInt(integerPart);
    debugPrintStr(".");
    debugPrintlnInt(decimalPart);
#elif 0
    debugPrintlnInt(toInt(n));
#endif
#endif
}

float applyPoly(const float *poly, int order, float x) {
    float result = 0.0;
    float tX = x;

    for (int i = 0; i <= order; ++i) {
        if (i == 0) {
            result += poly[i];
        } else {
            result += poly[i] * tX;
            tX *= x;
        }
    }
    return result;
}

float getShutterIsoEv() {
    int16_t x, y, z;
    if (magnetometerRead(&x, &y, &z) != 0) {
        debugPrintlnStr("Error reading magnetometer");
        return -99.0; // Error value
    }

#if 1
    debugPrintStr("Magnetometer\n\r X: ");
    debugPrintlnInt(x);
    debugPrintStr(" Y: ");
    debugPrintlnInt(y);
    debugPrintStr(" Z: ");
    debugPrintlnInt(z);
#endif

    // Process the magnetometer data to calculate EV
    //   Compute magnitude and direction (degrees) using X and Y axes
    // int magnitude = (int)sqrt((float)x * (float)x + (float)y * (float)y);
    // int direction = (int)atan2((float)y, (float)x) * 180.0 / PI; // Convert radians to degrees
    int magnitude = 0;
    int direction = 0;

#if 1
    debugPrintStr("  Magnitude: ");
    debugPrintlnInt(magnitude);
    debugPrintStr("  Direction (degrees): ");
    debugPrintlnInt(direction);
#endif

    //   Only consider readings greater than a certain threshold
    if (magnitude < 50) {
        return -99.0; // Return an error value
    }

    // Determine shutter+ISO EV based on direction
    return applyPoly(shutterIsoPoly, GET_POLY_ORDER(shutterIsoPoly), (float)direction);
}

// the setup function runs once when you press reset or power the board
void setup() {
#if SERIAL_DEBUG
    mySerial.begin(9600);
#endif

    analogReference(VDD);
    DACReference(INTERNAL1V1);

    int res;
    debugPrintStr("magnetometer setup...");
    res = magnetometerSetup();
    if (res != 0x40) {
        debugPrintStr("Failed ");
        debugPrintlnInt(res);
        while (1) {
            delay(1000);
        }
    }
}

// the loop function runs over and over again forever
void loop() {
#if 0
    // 1) Read the light input to get EV
    int lightInputAdcCounts = analogRead(CDS_INPUT_PIN);
    float evLightInput = applyPoly(lightInputPoly, GET_POLY_ORDER(lightInputPoly), (float)lightInputAdcCounts);
    debugPrintStr("Light Input: ");
    debugPrintInt(lightInputAdcCounts);
    debugPrintStr(" -> EV: ");
    debugPrintlnF(evLightInput);
#endif

    // 2) Read the current Shutter+ISO setting from magnetometer
    float evShutterISO = getShutterIsoEv();
    debugPrintStr("Shutter+ISO EV: ");
    debugPrintlnF(evShutterISO);

#if 0
    // 3) Calculate the meter output value (difference of light input EV and shutter+ISO EV)
    float evMeterOutput = evLightInput - evShutterISO;
    float meterOutputF = applyPoly(meterOutputPoly, GET_POLY_ORDER(meterOutputPoly), evMeterOutput);
    int meterOutput = (int)(meterOutputF);
    // Constrain the output to valid DAC range
    meterOutput = constrain(meterOutput, METER_OUTPUT_MIN_DAC, METER_OUTPUT_MAX_DAC);
    analogWrite(METER_OUTPUT_PIN, meterOutput);
    debugPrintStr("Final delta EV: ");
    debugPrintlnF(evMeterOutput);
    debugPrintStr("Meter Output: ");
    debugPrintlnInt(meterOutput);
#endif

    debugPrintStr("");

#if SERIAL_DEBUG
    delay(1000);
#endif
}
