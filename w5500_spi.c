#include "w5500_spi.h"
#include "stm32f4xx.h"

// W5500 RESET (PB0) 
void w5500_hw_reset(void)
{
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOBEN;

    GPIOB->MODER |= (1 << 0);   // PB0 output

    GPIOB->ODR &= ~(1 << 0);   // RESET LOW
    for (volatile int i = 0; i < 60000; i++);

    GPIOB->ODR |= (1 << 0);    // RESET HIGH
    for (volatile int i = 0; i < 60000; i++);
}

// Example GPIO control 
void w5500_cs_low(void)
{
    GPIOA->BSRR = (1 << (4 + 16));   // PA4 LOW
}

void w5500_cs_high(void)
{
    GPIOA->BSRR = (1 << 4);          // PA4 HIGH
}

// SPI byte transfer 
uint8_t w5500_spi_txrx(uint8_t data)
{
    while (!(SPI1->SR & SPI_SR_TXE));
    SPI1->DR = data;

    while (!(SPI1->SR & SPI_SR_RXNE));
    return SPI1->DR;
}

// Write 1 byte
void w5500_write(uint16_t addr, uint8_t block, uint8_t data)
{
    w5500_cs_low();

    // Address phase 
    w5500_spi_txrx(addr >> 8);
    w5500_spi_txrx(addr & 0xFF);

    // Control phase 
    w5500_spi_txrx((block << 3) | (1 << 2)); // Write, VDM

   //Data 
    w5500_spi_txrx(data);

    w5500_cs_high();
}

// Read 1 byte 
uint8_t w5500_read(uint16_t addr, uint8_t block)
{
    uint8_t ret;

    w5500_cs_low();

    w5500_spi_txrx(addr >> 8);
    w5500_spi_txrx(addr & 0xFF);

    w5500_spi_txrx((block << 3)); // Read, VDM

    ret = w5500_spi_txrx(0x00);

    w5500_cs_high();
    return ret;
}

void w5500_write_buf(uint16_t addr, uint8_t block, const uint8_t *buf, uint16_t len)
{
    w5500_cs_low();
    w5500_spi_txrx(addr >> 8);
    w5500_spi_txrx(addr & 0xFF);
    w5500_spi_txrx((block << 3) | (1 << 2)); // Write, VDM
    for (uint16_t i = 0; i < len; i++) {
        w5500_spi_txrx(buf[i]);
    }
    w5500_cs_high();
}

void w5500_read_buf(uint16_t addr, uint8_t block, uint8_t *buf, uint16_t len)
{
    w5500_cs_low();
    w5500_spi_txrx(addr >> 8);
    w5500_spi_txrx(addr & 0xFF);
    w5500_spi_txrx((block << 3)); // Read, VDM
    for (uint16_t i = 0; i < len; i++) {
        buf[i] = w5500_spi_txrx(0x00);
    }
    w5500_cs_high();
}
