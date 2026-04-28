#include "sh1106.h"

#include <stdint.h>
#include <stddef.h>
#include <unistd.h>
#include <sys/types.h>

/* SH1106 basic config */
#define SH1106_WIDTH       128
#define SH1106_PAGE_COUNT  8

/* SH1106 I2C control byte */
#define SH1106_CTRL_CMD    0x00
#define SH1106_CTRL_DATA   0x40

/* SH1106 command */
#define SH1106_CMD_DISPLAY_OFF  0xAE
#define SH1106_CMD_DISPLAY_ON   0xAF

typedef struct {
    char ch;
    uint8_t bitmap[5];
} font_char_t;

static int sh1106_send_command(int fd, uint8_t cmd);
static int sh1106_send_data(int fd, uint8_t data);
static int sh1106_set_page(int fd, uint8_t page);
static int sh1106_set_column(int fd, uint8_t col);
static int sh1106_fill(int fd, uint8_t pattern);
static const uint8_t *sh1106_get_font(char c);
static int sh1106_draw_char(int fd, char c);

/*
 * 5x7 font.
 * 目前只放這個專案會用到的字元。
 */
static const font_char_t font_table[] = {
    {' ', {0x00, 0x00, 0x00, 0x00, 0x00}},
    {'%', {0x62, 0x64, 0x08, 0x13, 0x23}},
    {'.', {0x00, 0x60, 0x60, 0x00, 0x00}},
    {'0', {0x3E, 0x51, 0x49, 0x45, 0x3E}},
    {'1', {0x00, 0x42, 0x7F, 0x40, 0x00}},
    {'2', {0x42, 0x61, 0x51, 0x49, 0x46}},
    {'3', {0x21, 0x41, 0x45, 0x4B, 0x31}},
    {'4', {0x18, 0x14, 0x12, 0x7F, 0x10}},
    {'5', {0x27, 0x45, 0x45, 0x45, 0x39}},
    {'6', {0x3C, 0x4A, 0x49, 0x49, 0x30}},
    {'7', {0x01, 0x71, 0x09, 0x05, 0x03}},
    {'8', {0x36, 0x49, 0x49, 0x49, 0x36}},
    {'9', {0x06, 0x49, 0x49, 0x29, 0x1E}},
    {':', {0x00, 0x36, 0x36, 0x00, 0x00}},
    {'C', {0x3E, 0x41, 0x41, 0x41, 0x22}},
    {'H', {0x7F, 0x08, 0x08, 0x08, 0x7F}},
    {'T', {0x01, 0x01, 0x7F, 0x01, 0x01}},
    {'e', {0x38, 0x54, 0x54, 0x54, 0x18}},
    {'i', {0x00, 0x44, 0x7D, 0x40, 0x00}},
    {'m', {0x7C, 0x04, 0x18, 0x04, 0x78}},
    {'p', {0x7C, 0x14, 0x14, 0x14, 0x08}},
    {'u', {0x3C, 0x40, 0x40, 0x20, 0x7C}},
};

/*
 * SH1106 initialization sequence.
 * 這裡使用 command table，比一長串 if 判斷更容易維護。
 */
static const uint8_t sh1106_init_cmds[] = {
    0xAE,       /* display off */
    0xD5, 0x80, /* display clock divide ratio / oscillator frequency */
    0xA8, 0x3F, /* multiplex ratio */
    0xD3, 0x00, /* display offset */
    0x40,       /* display start line */
    0xAD, 0x8B, /* charge pump */
    0xA1,       /* segment remap */
    0xC8,       /* COM scan direction */
    0xDA, 0x12, /* COM pins hardware configuration */
    0x81, 0xBF, /* contrast control */
    0xD9, 0x22, /* pre-charge period */
    0xDB, 0x40, /* VCOMH deselect level */
    0x32,       /* set Vpp */
    0xA6,       /* normal display */
    0xAF        /* display on */
};

int sh1106_init(int fd)
{
    if (fd < 0)
        return -1;

    for (size_t i = 0; i < sizeof(sh1106_init_cmds); i++) {
        if (sh1106_send_command(fd, sh1106_init_cmds[i]) != 0)
            return -1;
    }

    return 0;
}

int sh1106_clear(int fd)
{
    if (fd < 0)
        return -1;

    return sh1106_fill(fd, 0x00);
}

int sh1106_set_cursor(int fd, uint8_t page, uint8_t col)
{
    if (fd < 0)
        return -1;

    if (sh1106_set_page(fd, page) != 0)
        return -1;

    if (sh1106_set_column(fd, col) != 0)
        return -1;

    return 0;
}

int sh1106_draw_string(int fd, const char *str)
{
    if (fd < 0 || str == NULL)
        return -1;

    while (*str != '\0') {
        if (sh1106_draw_char(fd, *str) != 0)
            return -1;

        str++;
    }

    return 0;
}

int sh1106_poweroff(int fd)
{
    if (fd < 0)
        return -1;

    if (sh1106_send_command(fd, SH1106_CMD_DISPLAY_OFF) != 0)
        return -1;

    return 0;
}

int sh1106_poweron(int fd)
{
    if (fd < 0)
        return -1;

    if (sh1106_send_command(fd, SH1106_CMD_DISPLAY_ON) != 0)
        return -1;

    return 0;
}

static int sh1106_send_command(int fd, uint8_t cmd)
{
    uint8_t buf[2];

    if (fd < 0)
        return -1;

    buf[0] = SH1106_CTRL_CMD;
    buf[1] = cmd;

    if (write(fd, buf, sizeof(buf)) != (ssize_t)sizeof(buf))
        return -1;

    return 0;
}

static int sh1106_send_data(int fd, uint8_t data)
{
    uint8_t buf[2];

    if (fd < 0)
        return -1;

    buf[0] = SH1106_CTRL_DATA;
    buf[1] = data;

    if (write(fd, buf, sizeof(buf)) != (ssize_t)sizeof(buf))
        return -1;

    return 0;
}

static int sh1106_set_page(int fd, uint8_t page)
{
    if (fd < 0)
        return -1;

    if (page >= SH1106_PAGE_COUNT)
        return -1;

    return sh1106_send_command(fd, 0xB0 | page);
}

static int sh1106_set_column(int fd, uint8_t col)
{
    uint8_t low_col;
    uint8_t high_col;

    if (fd < 0)
        return -1;

    if (col >= SH1106_WIDTH)
        return -1;

    low_col = col & 0x0F;
    high_col = 0x10 | ((col >> 4) & 0x0F);

    if (sh1106_send_command(fd, low_col) != 0)
        return -1;

    if (sh1106_send_command(fd, high_col) != 0)
        return -1;

    return 0;
}

static int sh1106_fill(int fd, uint8_t pattern)
{
    if (fd < 0)
        return -1;

    for (uint8_t page = 0; page < SH1106_PAGE_COUNT; page++) {
        if (sh1106_set_page(fd, page) != 0)
            return -1;

        /*
         * 很多 1.3 inch SH1106 OLED module 會需要從 column 2 開始，
         * 才能對齊 128 pixel 顯示區。
         */
        if (sh1106_set_column(fd, 2) != 0)
            return -1;

        for (uint8_t col = 0; col < SH1106_WIDTH; col++) {
            if (sh1106_send_data(fd, pattern) != 0)
                return -1;
        }
    }

    return 0;
}

static const uint8_t *sh1106_get_font(char c)
{
    for (size_t i = 0; i < sizeof(font_table) / sizeof(font_table[0]); i++) {
        if (font_table[i].ch == c)
            return font_table[i].bitmap;
    }

    return font_table[0].bitmap; /* unknown char -> space */
}

static int sh1106_draw_char(int fd, char c)
{
    const uint8_t *bitmap;

    if (fd < 0)
        return -1;

    bitmap = sh1106_get_font(c);

    for (int i = 0; i < 5; i++) {
        if (sh1106_send_data(fd, bitmap[i]) != 0)
            return -1;
    }

    /* 字元間空一欄 */
    if (sh1106_send_data(fd, 0x00) != 0)
        return -1;

    return 0;
}
