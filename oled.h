#ifndef OLED_H
#define OLED_H

#include <stdint.h>

void oled_init(void);
void oled_clear(void);
void oled_set_cursor(uint8_t page, uint8_t col);
void oled_write_char(char c);
void oled_write_string(const char *str);

#endif

