#include "adxl345.h"
#include "i2c.h"

// ADXL345 Activity Init 

void adxl345_init_activity(void)
{
    // Measurement mode 
    i2c_write_reg(ADXL345_ADDR, POWER_CTL, 0x08);

    // Full resolution ±2g 
    i2c_write_reg(ADXL345_ADDR, DATA_FORMAT, 0x08);

    // Data rate 100 Hz
    //i2c_write_reg(ADXL345_ADDR, DATA_rate , 0x0A);

    // Enable activity on X, Y, Z (DC coupled) 
    i2c_write_reg(ADXL345_ADDR, ACT_INACT_CTL, 0x70);

    
    // THRESH_ACT = 0x04 → 2.0g 
    i2c_write_reg(ADXL345_ADDR, THRESH_ACT, 0x20);

    // Enable ACTIVITY interrupt 
    i2c_write_reg(ADXL345_ADDR, INT_ENABLE, 0x10);

    // Map ACTIVITY interrupt to INT1 
    i2c_write_reg(ADXL345_ADDR, INT_MAP, 0x00);
}

//  Read Interrupt Source  

uint8_t adxl345_read_int_source(void)
{
    return i2c_read_reg(ADXL345_ADDR, INT_SOURCE);
}


void adxl345_read_xyz(float *x, float *y, float *z)
{
    int16_t rx, ry, rz;

    adxl345_read_xyz(&rx, &ry, &rz);

    *x = rx * 0.0039f;
    *y = ry * 0.0039f;
    *z = rz * 0.0039f;
}
