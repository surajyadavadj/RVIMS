#ifndef GPS_H
#define GPS_H

#include <stdint.h>

void gps_init(void);
void gps_process(void);

uint8_t gps_fix_available(void);
float gps_get_lat(void);
float gps_get_lon(void);


uint8_t gps_get_hour(void);
uint8_t gps_get_min(void);
uint8_t gps_get_sec(void);
uint8_t gps_get_day(void);
uint8_t gps_get_month(void);
uint16_t gps_get_year(void);

#endif
