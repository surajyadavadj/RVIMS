/* usart_debug.c -- USART2 debug print functions
 */
#include "stdio.h"
#include "stdarg.h"
#include "stm32f411xe.h"
#include "usart_debug.h"

// Provide HSE/HSI fallback values if not defined elsewhere
#ifndef HSE_VALUE
#define HSE_VALUE 25000000U
#endif
#ifndef HSI_VALUE
#define HSI_VALUE 16000000U
#endif

/*
 * get_pclk1_freq -- return APB1 peripheral clock frequency (Hz)
 *
 * Detailed explanation:
 * This function reads RCC configuration registers to compute the current APB1
 * peripheral clock frequency (PCLK1). Many peripherals (including USART2)
 * are clocked from APB1 on STM32F4 devices; correct baud-rate/divider values
 * require knowing the actual PCLK1 value at runtime.
 *
 * Steps performed:
 * 1) Determine SYSCLK source:
 *    - If SWS indicates HSE, use the HSE_VALUE (external crystal).
 *    - If SWS indicates PLL, reconstruct the PLL output frequency from
 *      PLL configuration fields (PLLM, PLLN, PLLP) and the PLL source (PLLSRC).
 *      PLL output formula: SYSCLK = (PLL_input / PLLM) * PLLN / PLLP.
 *      PLLP encoding in PLLCFGR is 00->2, 01->4, 10->6, 11->8.
 *      Use 64-bit intermediate to avoid overflow during multiplication.
 *    - Otherwise assume HSI (HSI_VALUE).
 *
 * 2) Apply AHB prescaler (HPRE) to get HCLK from SYSCLK. HPRE bits map to
 *    either divide-by-1 for many encodings or to fixed divisors (2,4,8,...512).
 *    A small lookup table is used to map the HPRE field to an integer divisor.
 *
 * 3) Apply APB1 prescaler (PPRE1) to get PCLK1 from HCLK. The PPRE1 encoding
 *    is: values 0..3 => divide by 1; values 4..7 => divide by 2^(PPRE1-3).
 *
 * 4) Return PCLK1 as a 32-bit unsigned integer (Hz).
 *
 * Notes and assumptions:
 * - HSE_VALUE and HSI_VALUE must reflect the actual oscillator frequencies.
 * - Integer math is used; PLL intermediate uses uint64_t to reduce overflow risk.
 * - This routine reads RCC registers directly and therefore returns the
 *   currently configured clock; if the code is executed while PLL is
 *   not yet locked/active the result may not reflect a future clock change.
 */
static uint32_t get_pclk1_freq(void) {
    uint32_t sysclk;
    /* Read the System clock switch status to see which clock is providing SYSCLK */
    uint32_t sws = (RCC->CFGR & RCC_CFGR_SWS) >> RCC_CFGR_SWS_Pos;

    /* If SYSCLK source is HSE, use configured HSE_VALUE */
    if (sws == RCC_CFGR_SWS_HSE) {
        sysclk = HSE_VALUE;
    } else if (sws == RCC_CFGR_SWS_PLL) {
        /* SYSCLK is coming from PLL: reconstruct PLL output frequency */

        /* PLL source selection (HSE or HSI) */
        uint32_t pllsrc = (RCC->PLLCFGR & RCC_PLLCFGR_PLLSRC);

        /* PLLM: pre-divider for PLL input (bits PLLM[5:0]) */
        uint32_t pll_m = (RCC->PLLCFGR & RCC_PLLCFGR_PLLM) >> RCC_PLLCFGR_PLLM_Pos; 

        /* PLLN: main multiplier (bits PLLN[8:0]) */
        uint32_t pll_n = (RCC->PLLCFGR & RCC_PLLCFGR_PLLN) >> RCC_PLLCFGR_PLLN_Pos;

        /* PLLP: post-divider encoded as 00->2,01->4,10->6,11->8 (bits PLLP[1:0]) */
        uint32_t pll_p_bits = (RCC->PLLCFGR & RCC_PLLCFGR_PLLP) >> RCC_PLLCFGR_PLLP_Pos;

        /* Convert encoding to actual divider: value = 2 * (bits + 1) */
        uint32_t pll_p = 2U * (pll_p_bits + 1U); // encoding: 00->2,01->4,10->6,11->8

        /* Select PLL input clock frequency based on PLLSRC bit */
        uint32_t pll_input = (pllsrc == RCC_PLLCFGR_PLLSRC_HSE) ? HSE_VALUE : HSI_VALUE;

        /* Compute PLL output:
         *   V = (pll_input / PLLM) * PLLN / PLLP
         * Use 64-bit intermediate (v) to avoid overflow on multiplication.
         */
        uint64_t v = (uint64_t)pll_input * (uint64_t)pll_n;
        v /= pll_m;
        v /= pll_p;
        sysclk = (uint32_t)v;
    } else { /* SYSCLK source is HSI or unknown: default to HSI */
        sysclk = HSI_VALUE;
    }

    /* --- AHB prescaler (HPRE) handling --- */
    uint32_t hpre = (RCC->CFGR & RCC_CFGR_HPRE) >> RCC_CFGR_HPRE_Pos;
    /* Map HPRE field to actual AHB divider. For encodings 0..7 -> divide by 1,
       8..15 map to 2,4,8,16,64,128,256,512 respectively. */
    static const uint16_t ahb_div_table[16] = {
        1,1,1,1,1,1,1,1, /* 0..7 -> div1 */
        2,4,8,16,64,128,256,512 /* 8..15 */
    };
    uint32_t ahb_div = ahb_div_table[hpre & 0xF];

    /* --- APB1 prescaler (PPRE1) handling --- */
    uint32_t ppre1 = (RCC->CFGR & RCC_CFGR_PPRE1) >> RCC_CFGR_PPRE1_Pos;
    uint32_t apb1_div = 1;
    /* PPRE1 encoding: values 0..3 => div1, 4=>div2, 5=>div4, 6=>div8, 7=>div16.
       We compute it as (1 << (ppre1 - 3)) for ppre1>=4. */
    if (ppre1 < 4) {
        apb1_div = 1;
    } else {
        apb1_div = 1U << (ppre1 - 3); // 4->2,5->4,6->8,7->16
    }

    /* Now apply prescalers: HCLK = SYSCLK / AHB_div, PCLK1 = HCLK / APB1_div */
    uint32_t hclk = sysclk / ahb_div;
    uint32_t pclk1 = hclk / apb1_div;
    return pclk1;
}

// USART2 Send Character
static void USART2_SendChar(char ch) {
    while(!(USART2->SR & USART_SR_TXE));
    USART2->DR = ch;
}

// USART2 Initialization (115200 baud, includes GPIO setup)
void USART2_Init(void) {
    // Do not reconfigure system clock here — only enable peripheral clocks and compute BRR from current PCLK1.

    // Enable GPIOA and USART2 clocks
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;
    RCC->APB1ENR |= RCC_APB1ENR_USART2EN;

    // Configure USART2 pins (PA2=TX, PA3=RX) as alternate function
    GPIOA->MODER &= ~((3U << (2 * 2)) | (3U << (3 * 2)));
    GPIOA->MODER |= (2U << (2 * 2)) | (2U << (3 * 2));
    GPIOA->AFR[0] &= ~((0xF << (2 * 4)) | (0xF << (3 * 4)));
    GPIOA->AFR[0] |= (7U << (2 * 4)) | (7U << (3 * 4));  // AF7 for USART2

    // Configure baud rate (compute from current PCLK1)
    const uint32_t baud = 115200U;
    uint32_t pclk1 = get_pclk1_freq();
    // For oversampling by 16: BRR = PCLK1 / baud
    uint32_t brr = (pclk1 + (baud / 2U)) / baud; // rounded
    USART2->BRR = brr;

    // Enable TX and RX, enable USART
    USART2->CR1 = USART_CR1_TE | USART_CR1_RE | USART_CR1_UE;
}

// USART2 Debug print function (printf-style)
void usart_debug(const char* format, ...) {
    char buffer[256];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);

    // Send the formatted string
    for(int i = 0; buffer[i] != '\0'; i++) {
        USART2_SendChar(buffer[i]);
    }
}

