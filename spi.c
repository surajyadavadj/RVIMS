#include "spi.h"
#include "stm32f4xx.h"

/*
 SPI1 PINS (STM32F411)
 --------------------
 PA4  -> GPIO (CS handled outside)
 PA5  -> SPI1_SCK
 PA6  -> SPI1_MISO
 PA7  -> SPI1_MOSI
*/

void spi1_init(void)
{
    // Enable clocks 
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;
    RCC->APB2ENR |= RCC_APB2ENR_SPI1EN;

   

    // PA5, PA6, PA7 -> Alternate Function (SPI1) 
    GPIOA->MODER &= ~(0xFC00);      // clear MODER5,6,7
    GPIOA->MODER |=  (0xA800);      // AF mode

    // AF5 for SPI1 
    GPIOA->AFR[0] &= ~((0xF << 20) | (0xF << 24) | (0xF << 28));
    GPIOA->AFR[0] |=  (5 << 20) | (5 << 24) | (5 << 28);

    // PA4 as GPIO output (CS idle HIGH) 
    GPIOA->MODER &= ~(3 << 8);
    GPIOA->MODER |=  (1 << 8);
    GPIOA->ODR   |=  (1 << 4);

    // SPI CONFIG 

    SPI1->CR1 = 0;   // reset CR1

    SPI1->CR1 |=
        SPI_CR1_MSTR |     // Master mode
        SPI_CR1_SSM  |     // Software NSS
        SPI_CR1_SSI  |     // Internal NSS high
        SPI_CR1_BR_1 | SPI_CR1_BR_0;  // Baudrate = fPCLK/64 (safe start)

    /*
      SPI MODE 0
      CPOL = 0
      CPHA = 0
      MSB first (default)
      8-bit data (default)
    */

    SPI1->CR1 |= SPI_CR1_SPE;   // Enable SPI
}

// SPI1 transfer 
uint8_t spi1_txrx(uint8_t data)
{
    while (!(SPI1->SR & SPI_SR_TXE));
    SPI1->DR = data;

    while (!(SPI1->SR & SPI_SR_RXNE));
    return SPI1->DR;
}
