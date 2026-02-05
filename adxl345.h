#ifndef ADXL345_H
#define ADXL345_H

#include <stdint.h>

// ADXL345 I2C Address 
#define ADXL345_ADDR  0x53


#define THRESH_ACT      0x24
#define ACT_INACT_CTL   0x27
#define INT_ENABLE      0x2E
#define INT_MAP         0x2F
#define INT_SOURCE      0x30
#define POWER_CTL       0x2D
#define DATA_FORMAT     0x31
//#define DATA_rate       0x2C

void adxl345_init_activity(void);
uint8_t adxl345_read_int_source(void);
void adxl345_read_xyz(float *x, float *y, float *z);


#endif
