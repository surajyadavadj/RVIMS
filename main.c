#include "stm32f4xx.h"
#include "i2c.h"
#include "spi.h"
#include "w5500.h"
#include "adxl345.h"
#include "oled.h"
#include "gps.h"
#include <stdio.h>
#include <string.h>
#include <usart_debug.h>

volatile uint8_t activity_flag = 0;
volatile uint32_t ms_ticks = 0;

// DISPLAY CONTROL 
static uint8_t display_active = 0;
static uint32_t detect_time_ms = 0;

// SERVER CONFIG 
uint8_t SERVER_IP[4] = {192,168,1,106};   // PC IP
#define SERVER_PORT 5000

void SysTick_Handler(void)
{
    ms_ticks++;
}

// ADXL INTERRUPT
void EXTI0_IRQHandler(void)
{
    if (EXTI->PR & EXTI_PR_PR0)
    {
        activity_flag = 1;
        EXTI->PR = EXTI_PR_PR0;
    }
}


int main(void)
{
    char tcp_msg[128];
    char buf[64];

  
    USART2_Init();
    i2c1_init();
    spi1_init();

    oled_init();
    oled_clear();

    gps_init();
    usart_debug("GPS STARTED\r\n");

    w5500_hw_reset();
    w5500_net_config();

    w5500_tcp_client_connect(SERVER_IP, SERVER_PORT);
    while (!w5500_tcp_is_connected());
    usart_debug("TCP CONNECTED\r\n");

    SysTick_Config(SystemCoreClock / 1000);

    adxl345_init_activity();
    usart_debug("ADXL345 Activity > 2.0g\r\n");

  // EXTI PA0 
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;
    RCC->APB2ENR |= RCC_APB2ENR_SYSCFGEN;

    GPIOA->MODER &= ~(3 << (0 * 2));
    SYSCFG->EXTICR[0] = 0;
    EXTI->IMR  |= EXTI_IMR_IM0;
    EXTI->RTSR |= EXTI_RTSR_TR0;
    NVIC_EnableIRQ(EXTI0_IRQn);

    oled_set_cursor(0,0);
    oled_write_string("WAITING...");

    
    while (1)
    {
        gps_process();

        // ACTIVITY DETECT 
        if (activity_flag)
        {
            activity_flag = 0;

            if (adxl345_read_int_source() & 0x10)
            {
                if (!display_active)
                {
                    oled_clear();
                    oled_set_cursor(0,0);
                    oled_write_string("2G DETECTED");

                    usart_debug("2G DETECTED\r\n");

                    if (gps_fix_available())
                    {
                        snprintf(tcp_msg, sizeof(tcp_msg),
                            "EVENT:2G,LAT:%.4f,LON:%.4f,DATE:%02d-%02d-20%02d,TIME:%02d:%02d:%02d\r\n",
                            gps_get_lat(),
                            gps_get_lon(),
                            gps_get_day(),
                            gps_get_month(),
                            gps_get_year(),
                            gps_get_hour(),
                            gps_get_min(),
                            gps_get_sec()
                        );

                        oled_set_cursor(2,0);
                        snprintf(buf, sizeof(buf), "LAT %.4f", gps_get_lat());
                        oled_write_string(buf);

                        oled_set_cursor(3,0);
                        snprintf(buf, sizeof(buf), "LON %.4f", gps_get_lon());
                        oled_write_string(buf);
                    }
                    else
                    {
                        strcpy(tcp_msg, "EVENT:2G,LAT:NA,LON:NA,DATE:NA,TIME:NA\r\n");

                        oled_set_cursor(4,20);
                        oled_write_string("NO GPS FIX");
                    }

                    // SEND TO SERVER
                    w5500_tcp_send((uint8_t*)tcp_msg, strlen(tcp_msg));
                    usart_debug("TCP MSG SENT\r\n");

                    detect_time_ms = ms_ticks;
                    display_active = 1;
                }
            }
        }      
        if (display_active && ((ms_ticks - detect_time_ms) >= 2000))
        {
            oled_clear();
            oled_set_cursor(4,50);
            oled_write_string("WAITING...");
            display_active = 0;
        }
    }
}