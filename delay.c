#include "delay.h"

void delay_ms(uint32_t ms)
{
    volatile uint32_t count;
    while(ms--)
    {
        count = 16000;   // ~1ms @ 16MHz (approx, safe for W5500)
        while(count--) __asm__("nop");
    }
}
