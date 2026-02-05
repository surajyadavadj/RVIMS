#include "stm32f4xx.h"
#include "i2c.h"

void i2c1_init(void)
{
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOBEN;
    RCC->APB1ENR |= RCC_APB1ENR_I2C1EN;

    GPIOB->MODER |= (2<< 16) | (2<< 18);
    GPIOB->OTYPER |= (1<<8) | (1<<9);
    GPIOB->AFR[1] |= (4<< 0) | (4<< 4);

    I2C1->CR2 = 16;  // PCLK1 = 16 MHz
    I2C1->CCR = 80;  // 100 kHz
    I2C1->TRISE = 17;  // (16/1)+1
    I2C1->CR1 |= I2C_CR1_PE;  // Enable I2C
}

void i2c_write_reg(uint8_t dev, uint8_t reg, uint8_t data)
{
    // START
    I2C1->CR1 |= I2C_CR1_START;
    while (!(I2C1->SR1 & I2C_SR1_SB));

    // Slave address + Write
    I2C1->DR = dev << 1;
    while (!(I2C1->SR1 & I2C_SR1_ADDR));
    (void)I2C1->SR2; // Clear ADDR


    // Send data
    I2C1->DR = reg;
    while (!(I2C1->SR1 & I2C_SR1_TXE));
    I2C1->DR = data;
    while (!(I2C1->SR1 & I2C_SR1_BTF));

    // STOP
    I2C1->CR1 |= I2C_CR1_STOP;
}

uint8_t i2c_read_reg(uint8_t dev, uint8_t reg)
{
    uint8_t val;

    I2C1->CR1 |= I2C_CR1_START;
    while (!(I2C1->SR1 & I2C_SR1_SB));
    I2C1->DR = dev << 1;
    while (!(I2C1->SR1 & I2C_SR1_ADDR));
    (void)I2C1->SR2;

    I2C1->DR = reg;
    while (!(I2C1->SR1 & I2C_SR1_TXE));

    I2C1->CR1 |= I2C_CR1_START;
    while (!(I2C1->SR1 & I2C_SR1_SB));
    I2C1->DR = (dev << 1) | 1;
    while (!(I2C1->SR1 & I2C_SR1_ADDR));
    (void)I2C1->SR2;

    I2C1->CR1 &= ~I2C_CR1_ACK;
    while (!(I2C1->SR1 & I2C_SR1_RXNE));
    val = I2C1->DR;

    I2C1->CR1 |= I2C_CR1_STOP;
    I2C1->CR1 |= I2C_CR1_ACK;

    return val;
}
