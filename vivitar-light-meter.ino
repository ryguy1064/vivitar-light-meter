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
const float meterOutputPoly[] = {103.0, 30.4};

#define GET_POLY_ORDER(x) (sizeof(x) / sizeof(x[0]) - 1)

// Debug
#if SERIAL_DEBUG
TXOnlySerial mySerial(DEBUG_OUTPUT_PIN);
#endif
// LIS2MDLSensor Magneto(&Wire);

uint8_t value = 0;
const uint8_t delta = 1;
bool dir = true;

int analogReading;
float test;

void debugPrint(const char *str) {
#if SERIAL_DEBUG
    mySerial.print(str);
#endif
}

void debugPrintln(const char *str) {
#if SERIAL_DEBUG
    mySerial.println(str);
#endif
}

void debugPrint(int n) {
#if SERIAL_DEBUG
    mySerial.print(n);
#endif
}

void debugPrintln(int n) {
#if SERIAL_DEBUG
    mySerial.println(n);
#endif
}

void debugPrintln(float n) {
#if SERIAL_DEBUG
    // Cannot use standard print with floating-point libraries due to limited flash space,
    // so this implements a simple float to string conversion with 1 decimal place.
    if (n < 0.0) {
        mySerial.print('-');
        n = -n; // Make it positive for processing
    }
    int integerPart = (int)n;   // truncate
    int decimalPart = (int)((n - (float)integerPart) * 10.0); // Get the first decimal place
    mySerial.print(integerPart);
    mySerial.print('.');
    mySerial.print(decimalPart);
    mySerial.println();
#endif
}

float applyPoly(const float *poly, int order, float x) {
    float result = 0.0f;
    float tX = x;

    // debugPrintln("applyPoly");
    for (int i = 0; i <= order; ++i) {
        if (i == 0) {
            result += poly[i];
        } else {
            result += poly[i] * tX;
            tX *= x;
        }
        // debugPrintln(result);
    }
    // debugPrintln("---------");
    return result;
}

float getShutterIsoEv() {

    return 0.0;
}

// the setup function runs once when you press reset or power the board
void setup() {
    // initialize digital pin LED_BUILTIN as an output.
    // pinMode(LED_BUILTIN, OUTPUT);

#if SERIAL_DEBUG
    mySerial.begin(9600);
#endif

    analogReference(VDD);
    DACReference(INTERNAL1V1);

    magnetometerSetup();
}

// the loop function runs over and over again forever
void loop() {
    // 1) Read the light input to get EV
    int lightInputAdcCounts = analogRead(CDS_INPUT_PIN);
    float evLightInput = applyPoly(lightInputPoly, GET_POLY_ORDER(lightInputPoly), (float)lightInputAdcCounts);
    debugPrint("Light Input: ");
    debugPrint(lightInputAdcCounts);
    debugPrint(" -> EV: ");
    // mySerial.println(evLightInput, 2);
    debugPrintln(evLightInput);

    // 2) Read the current Shutter+ISO setting from magnetometer
    float evShutterISO = 8.0; // Placeholder for shutter+ISO EV value
    // evShutterISO = getShutterIsoEv();
    debugPrint("Shutter+ISO EV: ");
    // mySerial.println(evShutterISO, 2);
    debugPrintln(evShutterISO);

    // 3) Calculate the meter output value (difference of light input EV and shutter+ISO EV)
    float evMeterOutput = evLightInput - evShutterISO;
    int meterOutput = (int)applyPoly(meterOutputPoly, GET_POLY_ORDER(meterOutputPoly), evMeterOutput);
    // Constrain the output to valid DAC range
    meterOutput = constrain(meterOutput, METER_OUTPUT_MIN_DAC, METER_OUTPUT_MAX_DAC);
    analogWrite(METER_OUTPUT_PIN, meterOutput);
    debugPrint("Final delta EV: ");
    debugPrintln(evMeterOutput);
    debugPrint("Meter Output: ");
    debugPrintln(meterOutput);

    debugPrintln("");

#if SERIAL_DEBUG
    delay(1000);
#endif

#if 0
    analogReading = analogRead(PIN_A3);
    // mySerial.println(analogReading);
    test = analogReading;
    test *= 3.14159;
#if SERIAL_DEBUG
    mySerial.println(test, 2);
#endif
    analogReading = (int)test;
#if SERIAL_DEBUG
    mySerial.println(analogReading);
#endif

    delay(1000);
#endif

#if 0
    analogWrite(PIN_A6, value);
    if (dir)
    value += delta;
    else
    value -= delta;

    if (value == 0 || value == 255)
    dir = !dir;

    delay(5);
#endif
}
