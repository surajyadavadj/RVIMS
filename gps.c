#include "gps.h"
#include "stm32f4xx.h"
#include <string.h>
#include <stdlib.h>

// CONFIG 
#define GPS_BUF_SIZE 128

// GLOBAL
static char gps_line[GPS_BUF_SIZE];
static uint8_t gps_idx = 0;

static float gps_latitude = 0.0f;
static float gps_longitude = 0.0f;
static uint8_t gps_fix = 0;

// DATE & TIME (IST) 
static uint8_t gps_h, gps_m, gps_s;
static uint8_t gps_d, gps_mo;
static uint16_t gps_y;

// INTERNAL
static float nmea_to_decimal(char *val, char dir)
{
    float raw = atof(val);
    int deg = (int)(raw / 100);
    float min = raw - (deg * 100);
    float dec = deg + (min / 60.0f);

    if (dir == 'S' || dir == 'W')
        dec = -dec;

    return dec;
}

static void gps_parse_gprmc(char *s)
{
    char *tok;

    tok = strtok(s, ",");        // $GPRMC
    tok = strtok(NULL, ",");     // UTC time hhmmss

    if (tok && strlen(tok) >= 6)
    {
        gps_h = (tok[0]-'0')*10 + (tok[1]-'0');
        gps_m = (tok[2]-'0')*10 + (tok[3]-'0');
        gps_s = (tok[4]-'0')*10 + (tok[5]-'0');
    }

    tok = strtok(NULL, ",");     // status
    if (!tok || tok[0] != 'A')
    {
        gps_fix = 0;
        return;
    }

    char *lat = strtok(NULL, ",");
    char *lat_dir = strtok(NULL, ",");
    char *lon = strtok(NULL, ",");
    char *lon_dir = strtok(NULL, ",");

    strtok(NULL, ","); // speed
    strtok(NULL, ","); // course

    tok = strtok(NULL, ",");     // date ddmmyy
    if (tok && strlen(tok) == 6)
    {
        gps_d  = (tok[0]-'0')*10 + (tok[1]-'0');
        gps_mo = (tok[2]-'0')*10 + (tok[3]-'0');
        gps_y  = 2000 + (tok[4]-'0')*10 + (tok[5]-'0');
    }

    gps_latitude  = nmea_to_decimal(lat, lat_dir[0]);
    gps_longitude = nmea_to_decimal(lon, lon_dir[0]);

    // UTC → IST (+5:30) 
    gps_h += 5;
    gps_m += 30;
    if (gps_m >= 60) { gps_m -= 60; gps_h++; }
    if (gps_h >= 24) { gps_h -= 24; gps_d++; }

    gps_fix = 1;
}

// PUBLIC API

void gps_init(void)
{
    RCC->APB2ENR |= RCC_APB2ENR_USART6EN;
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOCEN;

    GPIOC->MODER |= (2 << (6 * 2)) | (2 << (7 * 2));
    GPIOC->AFR[0] |= (8 << (6 * 4)) | (8 << (7 * 4));

    USART6->BRR = 0x0683; // 9600 baud
    USART6->CR1 |= USART_CR1_RE | USART_CR1_UE;
}

void gps_process(void)
{
    char c;

    while (USART6->SR & USART_SR_RXNE)
    {
        c = USART6->DR;

        if (c == '\n')
        {
            gps_line[gps_idx] = 0;
            if (strstr(gps_line, "$GPRMC"))
                gps_parse_gprmc(gps_line);
            gps_idx = 0;
        }
        else if (gps_idx < GPS_BUF_SIZE - 1)
        {
            gps_line[gps_idx++] = c;
        }
    }
}

uint8_t gps_fix_available(void) { return gps_fix; }
float gps_get_lat(void) { return gps_latitude; }
float gps_get_lon(void) { return gps_longitude; }

uint8_t gps_get_hour(void) { return gps_h; }
uint8_t gps_get_min(void) { return gps_m; }
uint8_t gps_get_sec(void) { return gps_s; }
uint8_t gps_get_day(void) { return gps_d; }
uint8_t gps_get_month(void) { return gps_mo; }
uint16_t gps_get_year(void) { return gps_y; }
