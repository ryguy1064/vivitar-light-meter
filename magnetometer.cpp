#include "magnetometer.h"
#include <Wire.h>
#include <TXOnlySerial.h>

#define MAG_I2C_ADDRESS         0x1E // 0011110b + R/W

extern TXOnlySerial mySerial;

// Registers
#define MAG_REG_WHO_AM_I        0x4F
#define MAG_REG_CFG_REG_A       0x60
#define MAG_REG_CFG_REG_B       0x61
#define MAG_REG_CFG_REG_C       0x62
#define MAG_REG_STATUS          0x67
#define MAG_REG_OUT_X_L         0x68
#define MAG_REG_OUT_X_H         0x69
#define MAG_REG_OUT_Y_L         0x6A
#define MAG_REG_OUT_Y_H         0x6B
#define MAG_REG_OUT_Z_L         0x6C
#define MAG_REG_OUT_Z_H         0x6D

// Configuration values
#define MAG_CFG_REG_A_DEFAULT   0x80 // 10000000b // High-res mode, ODR 10Hz, continuous mode
#define MAG_CFG_REG_B_DEFAULT   0x00 // 00000001b // Default settings, low-pass filter
#define MAG_CFG_REG_C_DEFAULT   0x00 // 00000000b // Default settings, no self-test, no BDU

// Status register bits
#define MAG_STATUS_ZYXDA        0x08 // Data available for all axes

static int ioRead(uint8_t reg, uint8_t *data, uint16_t len) {
    Wire.beginTransmission(MAG_I2C_ADDRESS);
    Wire.write(reg);
    Wire.endTransmission(false); // Send a repeated start
    Wire.requestFrom(MAG_I2C_ADDRESS, len);

    for (uint16_t i = 0; i < len; i++) {
        if (Wire.available()) {
            // mySerial.print("b");
            data[i] = Wire.read();
            // mySerial.println(data[i], HEX);
        } else {
            // mySerial.print("x");
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
    ioRead(MAG_REG_WHO_AM_I, data, 1);

    mySerial.print("WHO_AM_I: ");
    mySerial.println(data[0], HEX);

    return data[0];
}

int magnetometerRead(int16_t *x, int16_t *y, int16_t *z) {
    // 1. Start a continuous measurement
    uint8_t cfgA = MAG_CFG_REG_A_DEFAULT;
    // mySerial.println("Start single measurement");
    if (ioWrite(MAG_REG_CFG_REG_A, &cfgA, 1) != 0) {
        // mySerial.println("Error writing CFG_REG_A");
        return -1;
    }

    uint8_t cfgB = MAG_CFG_REG_B_DEFAULT;
    if (ioWrite(MAG_REG_CFG_REG_B, &cfgB, 1) != 0) {
        return -1;
    }

    // 2. Wait for data to be ready
    uint8_t status;
    do {
        mySerial.println("Waiting for data ready...");
        if (ioRead(MAG_REG_STATUS, &status, 1) != 0) {
            // mySerial.println("Error reading status");
            return -1;
        }
        delay(100); // Wait a bit before checking again
    } while (!(status & MAG_STATUS_ZYXDA)); // Wait until data is available

    // 3. Read the magnetic data
    uint8_t rawData[6];
    mySerial.println("Reading magnetic data...");
    if (ioRead(MAG_REG_OUT_X_L, rawData, 6) != 0) {
        // mySerial.println("Error reading magnetic data");
        return -1;
    }
    *x = (int16_t)((rawData[1] << 8) | rawData[0]);
    *y = (int16_t)((rawData[3] << 8) | rawData[2]);
    *z = (int16_t)((rawData[5] << 8) | rawData[4]);

    return 0;
}
