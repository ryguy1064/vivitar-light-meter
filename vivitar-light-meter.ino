/**
 * Program to convert my Vivitar 400/SL 35mm film SLR camera's inaccurate all-analog light meter system 
 * to a microcontroller-based system. It was calibrated against my Canon Rebel GII using a light box.
 * 
 * Uses the original pair of CdS light sensors in the viewfinder and the original light meter gauge
 * with its mechanical linkage to the shutter/ISO dial, which provides increments at the stop level.
 * This is all powered by 3V lithium battery (CR1632) as the original coin cell holder doesn't provide
 * enough voltage.
 * 
 * The code uses Light Value (EV) instead of Exposure Value (EV), which are functionally equivalent at
 * ISO 100. LV is preferred as the aperture, shutter, and ISO are not directly known by the system
 * (only the input light level in the viewfinder).
 * 
 * Designed to be run on an ATtiny412 microcontroller since it has a built-in 10-bit ADC and 8-bit DAC.
 * 
 * Future improvement plans:
 * - Since the CdS sensors can be slow to respond to falling light levels, this adds extra time needed
 *   when taking photos. So there is the possibility of changing to a digital lux sensor (VEML7700-TT)
 *   that communicates over an I2C interface (also supported by the ATtiny412). The sensor only goes down
 *   to 2.5V, but that still should provide a long life according to the battery's datasheet.
 * 
 * Author: Ryan Warner
 * 
 * -------------------
 * 
 * MIT License
 * 
 * Copyright (c) 2025 ryguy1064
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <Arduino.h>
#include <TXOnlySerial.h>

#define SERIAL_DEBUG        1
#define VERSION_STR         "v1.0.0"

#define DEBUG_OUTPUT_PIN    PIN_A7
#define CDS_INPUT_PIN       PIN_A3
#define METER_OUTPUT_PIN    PIN_A6
#define METER_OUTPUT_MIN_DAC    0   // 0.00V
#define METER_OUTPUT_MAX_DAC    204 // 0.88V

// Source of coefficients: 
// https://docs.google.com/spreadsheets/d/10hfEoG8dDYRyn8PrnT6vj_a-6OrD_DNj-Ta4taBO-kc/edit?usp=sharing

// Light ADC counts -> Light Value (LV)
const float lightInputPoly[] = {3.6, 0.0417, -9.43e-5, 1.04e-7, -3.68e-11};

// Tweak to LV post-input (based on field observations)
const float lvTweak = -0.5;

// LV -> meter DAC counts
const float meterOutputPoly[] = {-2.88, 4.53, -0.213, 0.113, -4.48e-3};

#define GET_POLY_ORDER(x) (sizeof(x) / sizeof(x[0]) - 1)

// Debug
#if SERIAL_DEBUG
TXOnlySerial debugSerial(DEBUG_OUTPUT_PIN);
#endif

void debugPrintStr(const char *str) {
#if SERIAL_DEBUG
    debugSerial.print(str);
#endif
}

void debugPrintlnStr(const char *str) {
#if SERIAL_DEBUG
    debugSerial.println(str);
#endif
}

void debugPrintInt(int n) {
#if SERIAL_DEBUG
    debugSerial.print(n);
#endif
}

void debugPrintlnInt(int n) {
#if SERIAL_DEBUG
    debugSerial.println(n);
#endif
}

void debugPrintlnF(float n) {
#if SERIAL_DEBUG
    // Cannot use standard print with floating-point libraries due to limited flash space,
    // so this implements a simple float to string conversion with 1 decimal place.
    if (n < 0.0) {
        debugPrintStr("-");
        n = -n; // Make it positive for processing
    }
    int integerPart = (int)n;   // truncate
    int decimalPart = (int)((n - (float)integerPart) * 10.0); // Get the first decimal place
    debugPrintInt(integerPart);
    debugPrintStr(".");
    debugPrintlnInt(decimalPart);
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

void setup() {
#if SERIAL_DEBUG
    debugSerial.begin(9600);

    debugPrintlnStr("Digitally-controlled light meter " VERSION_STR);
#endif

    // Use VDD as reference so accuracy of LV input is maintained as battery drains
    analogReference(VDD);

    // Meter's max is 0.88V, so a 1.1V reference is sufficient
    DACReference(INTERNAL1V1);
}

void loop() {
    // 1) Read the CdS sensor input to get LV
    int lightInputCounts = analogRead(CDS_INPUT_PIN);
    float lvLightInput = applyPoly(lightInputPoly, GET_POLY_ORDER(lightInputPoly), (float)lightInputCounts);

    // 2) Perform tweak
    lvLightInput += lvTweak;

    debugPrintStr("Light Input: ");
    debugPrintInt(lightInputCounts);
    debugPrintStr(" -> LV: ");
    debugPrintlnF(lvLightInput);

    // 3) Calculate the meter output (DAC counts) from LV
    int meterOutputCounts = (int)applyPoly(meterOutputPoly, GET_POLY_ORDER(meterOutputPoly), lvLightInput);
    //   Constrain the meter output to valid DAC range
    meterOutputCounts = constrain(meterOutputCounts, METER_OUTPUT_MIN_DAC, METER_OUTPUT_MAX_DAC);
    analogWrite(METER_OUTPUT_PIN, meterOutputCounts);

    debugPrintStr("LV -> Meter Output: ");
    debugPrintlnInt(meterOutputCounts);
    debugPrintlnStr("");

#if SERIAL_DEBUG
    delay(100);
#endif
}
