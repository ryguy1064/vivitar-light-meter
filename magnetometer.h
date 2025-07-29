#ifndef MAGNETOMETER_H
#define MAGNETOMETER_H

#include <Arduino.h>

int magnetometerSetup();

int magnetometerRead(int16_t *x, int16_t *y, int16_t *z);



#endif // MAGNETOMETER_H