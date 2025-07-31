#include <Arduino.h>
#include <TXOnlySerial.h>

#define SERIAL_DEBUG 1

#define DEBUG_OUTPUT_PIN    PIN_A7
#define CDS_INPUT_PIN       PIN_A3
#define METER_OUTPUT_PIN    PIN_A6
#define METER_OUTPUT_MIN_DAC    0
#define METER_OUTPUT_MAX_DAC    185

// Source of coefficients: https://docs.google.com/spreadsheets/d/10hfEoG8dDYRyn8PrnT6vj_a-6OrD_DNj-Ta4taBO-kc/edit?usp=sharing

// Light ADC counts -> EV
const float lightInputPoly[] = {3.6, 0.0417, -9.43e-5, 1.04e-7, -3.68e-11};

const float lvAdjust = -0.5;

// EV -> meter DAC counts
const float meterOutputPoly[] = {-2.88, 4.53, -0.213, 0.113, -4.48e-3};

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

// the setup function runs once when you press reset or power the board
void setup() {
#if SERIAL_DEBUG
    mySerial.begin(9600);
#endif

    analogReference(VDD);
    DACReference(INTERNAL1V1);
}

// the loop function runs over and over again forever
void loop() {
    // 1) Read the light input to get EV
    int lightInputAdcCounts = analogRead(CDS_INPUT_PIN);
    float evLightInput = applyPoly(lightInputPoly, GET_POLY_ORDER(lightInputPoly), (float)lightInputAdcCounts);
    debugPrintStr("Light Input: ");
    debugPrintInt(lightInputAdcCounts);
    debugPrintStr(" -> EV: ");
    debugPrintlnF(evLightInput);

    // 2) Perform adjustment
    evLightInput += lvAdjust;

    // 2) Calculate the meter output value from EV
    int meterOutput = (int)applyPoly(meterOutputPoly, GET_POLY_ORDER(meterOutputPoly), evLightInput);
    // Constrain the output to valid DAC range
    meterOutput = constrain(meterOutput, METER_OUTPUT_MIN_DAC, METER_OUTPUT_MAX_DAC);
    analogWrite(METER_OUTPUT_PIN, meterOutput);
    debugPrintStr("EV -> Meter Output: ");
    debugPrintlnInt(meterOutput);

    debugPrintlnStr("");

#if SERIAL_DEBUG
    delay(100);
#endif
}
