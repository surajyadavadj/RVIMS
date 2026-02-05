#ifndef I2C_H
#define I2C_H

#include <stdint.h>

void i2c1_init(void);
void i2c_write_reg(uint8_t dev, uint8_t reg, uint8_t data);
uint8_t i2c_read_reg(uint8_t dev, uint8_t reg);

#endif
