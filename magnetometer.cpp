#include "magnetometer.h"
#include <Arduino.h>
#include <Wire.h>
#include <TXOnlySerial.h>

#define MAG_I2C_ADDRESS         0x1E // 0011110b + R/W

extern TXOnlySerial mySerial;

// Registers
#define MAG_REG_WHO_AM_I        0x4F

static int ioRead(uint8_t reg, uint8_t *data, uint16_t len) {
    Wire.beginTransmission(MAG_I2C_ADDRESS);
    Wire.write(reg);
    Wire.endTransmission(false); // Send a repeated start
    Wire.requestFrom(MAG_I2C_ADDRESS, len);

    for (uint16_t i = 0; i < len; i++) {
        if (Wire.available()) {
            data[i] = Wire.read();
        } else {
            return -1; // Error: not enough data
        }
    }
    return 0; // Success
}

static int ioWrite(uint8_t reg, uint8_t *data, uint16_t len) {
    Wire.beginTransmission(MAG_I2C_ADDRESS);
    Wire.write(reg);
    for (uint16_t i = 0; i < len; i++) {
        Wire.write(data[i]);
    }
    return Wire.endTransmission(); // Returns 0 on success
}

int magnetometerSetup() {
    uint8_t data[16];

    Wire.begin();

    // Check the WHO_AM_I register (should be 0x40)
    ioRead(MAG_REG_WHO_AM_I, data, 1); // Clear any previous data

    mySerial.print("WHO_AM_I: ");
    mySerial.println(data[0], HEX);
}


