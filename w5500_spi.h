#ifndef _W5500_SPI_H_
#define _W5500_SPI_H_

#include <stdint.h>
// GPIO macros 
#define W5500_RST_LOW()   GPIOB->BSRR = (1<<(0+16))   // example PB0
#define W5500_RST_HIGH()  GPIOB->BSRR = (1<<0)

void w5500_hw_reset(void);
// CS control 
void w5500_cs_low(void);
void w5500_cs_high(void);

uint8_t w5500_spi_txrx(uint8_t data);
void w5500_write(uint16_t addr, uint8_t block, uint8_t data);
uint8_t w5500_read(uint16_t addr, uint8_t block);
void w5500_write_buf(uint16_t addr, uint8_t block, const uint8_t *buf, uint16_t len);
void w5500_read_buf (uint16_t addr, uint8_t block, uint8_t *buf, uint16_t len);

#endif
