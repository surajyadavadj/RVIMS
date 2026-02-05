#ifndef _SPI_H_
#define _SPI_H_

#include <stdint.h>
void spi1_init(void);
uint8_t spi1_txrx(uint8_t data);
#endif
