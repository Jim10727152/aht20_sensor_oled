#ifndef SH1106_H
#define SH1106_H

#include <stdint.h>

int sh1106_init(int fd);
int sh1106_clear(int fd);
int sh1106_set_cursor(int fd, uint8_t page, uint8_t col);
int sh1106_draw_string(int fd, const char *str);
int sh1106_poweroff(int fd);

#endif
