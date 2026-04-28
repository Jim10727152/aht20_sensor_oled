#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>
#include <string.h>

#define I2C_DEVICE "/dev/i2c-1"

/* AHT20 */
#define AHT20_ADDR 0x38
#define AHT20_CMD_STATUS 0x71
#define AHT20_CMD_MEASURE 0xAC
#define AHT20_ARG_MEASURE1 0x33
#define AHT20_ARG_MEASURE2 0x00
#define AHT20_STATUS_BUSY 0x80
#define AHT20_STATUS_READY 0x18
#define AHT20_DATA_LEN 6

/* SH1106 */
#define SH1106_ADDR 0x3C

int aht20_read_status(int fd, uint8_t *status);
int aht20_trigger_measurement(int fd);
int aht20_read_data(int fd, uint8_t *data, size_t len);
float aht20_parse_temperature(const uint8_t *data);
float aht20_parse_humidity(const uint8_t *data);
int aht20_read_temperature_humidity(int fd, float *temp, float *humi);

int sh1106_send_command(int fd, uint8_t cmd);
int sh1106_send_data(int fd, uint8_t data);
int sh1106_init(int fd);
int sh1106_set_page(int fd, uint8_t page);
int sh1106_set_column(int fd, uint8_t col);
int sh1106_clear(int fd);
int sh1106_fill(int fd, uint8_t pattern);
int sh1106_set_cursor(int fd, uint8_t page, uint8_t col);
int sh1106_draw_char(int fd, char c);
int sh1106_draw_string(int fd, const char *str);
int get_font_index(char c);

/* 只放目前需要的字元 */
const uint8_t font[][5] = {
    /* ' ' */
    {0x00, 0x00, 0x00, 0x00, 0x00},
    /* '%' */
    {0x62, 0x64, 0x08, 0x13, 0x23},
    /* '.' */
    {0x00, 0x60, 0x60, 0x00, 0x00},
    /* '0' */
    {0x3E, 0x51, 0x49, 0x45, 0x3E},
    /* '1' */
    {0x00, 0x42, 0x7F, 0x40, 0x00},
    /* '2' */
    {0x42, 0x61, 0x51, 0x49, 0x46},
    /* '3' */
    {0x21, 0x41, 0x45, 0x4B, 0x31},
    /* '4' */
    {0x18, 0x14, 0x12, 0x7F, 0x10},
    /* '5' */
    {0x27, 0x45, 0x45, 0x45, 0x39},
    /* '6' */
    {0x3C, 0x4A, 0x49, 0x49, 0x30},
    /* '7' */
    {0x01, 0x71, 0x09, 0x05, 0x03},
    /* '8' */
    {0x36, 0x49, 0x49, 0x49, 0x36},
    /* '9' */
    {0x06, 0x49, 0x49, 0x29, 0x1E},
    /* ':' */
    {0x00, 0x36, 0x36, 0x00, 0x00},
    /* 'C' */
    {0x3E, 0x41, 0x41, 0x41, 0x22},
    /* 'H' */
    {0x7F, 0x08, 0x08, 0x08, 0x7F},
    /* 'T' */
    {0x01, 0x01, 0x7F, 0x01, 0x01},
    /* 'e' */
    {0x38, 0x54, 0x54, 0x54, 0x18},
    /* 'i' */
    {0x00, 0x44, 0x7D, 0x40, 0x00},
    /* 'm' */
    {0x7C, 0x04, 0x18, 0x04, 0x78},
    /* 'p' */
    {0x7C, 0x14, 0x14, 0x14, 0x08},
    /* 'u' */
    {0x3C, 0x40, 0x40, 0x20, 0x7C}};

/* =========================
 * AHT20
 * ========================= */
int aht20_read_status(int fd, uint8_t *status)
{
    uint8_t cmd = AHT20_CMD_STATUS;

    if (status == NULL)
        return -1;

    if (write(fd, &cmd, 1) != 1)
        return -1;

    if (read(fd, status, 1) != 1)
        return -1;

    return 0;
}

int aht20_trigger_measurement(int fd)
{
    uint8_t cmd[3] = {
        AHT20_CMD_MEASURE,
        AHT20_ARG_MEASURE1,
        AHT20_ARG_MEASURE2};

    if (write(fd, cmd, 3) != 3)
        return -1;

    return 0;
}

int aht20_read_data(int fd, uint8_t *data, size_t len)
{
    uint8_t cmd = AHT20_CMD_STATUS;

    if (data == NULL)
        return -1;

    if (write(fd, &cmd, 1) != 1)
        return -1;

    if (read(fd, data, len) != (ssize_t)len)
        return -1;

    return 0;
}

float aht20_parse_temperature(const uint8_t *data)
{
    uint32_t raw_temp = ((data[3] & 0x0F) << 16) | (data[4] << 8) | data[5];
    return (raw_temp / 1048576.0f) * 200.0f - 50.0f;
}

float aht20_parse_humidity(const uint8_t *data)
{
    uint32_t raw_humi = (data[1] << 12) | (data[2] << 4) | ((data[3] & 0xF0) >> 4);
    return (raw_humi / 1048576.0f) * 100.0f;
}

int aht20_read_temperature_humidity(int fd, float *temp, float *humi)
{
    uint8_t status;
    uint8_t data[AHT20_DATA_LEN];

    if (temp == NULL || humi == NULL)
        return -1;

    if (aht20_read_status(fd, &status) != 0)
        return -1;

    if ((status & AHT20_STATUS_READY) != AHT20_STATUS_READY)
        return -1;

    usleep(100000);

    if (aht20_trigger_measurement(fd) != 0)
        return -1;

    do
    {
        usleep(80000);
        if (aht20_read_status(fd, &status) != 0)
            return -1;
    } while ((status & AHT20_STATUS_BUSY) != 0);

    if (aht20_read_data(fd, data, AHT20_DATA_LEN) != 0)
        return -1;

    *temp = aht20_parse_temperature(data);
    *humi = aht20_parse_humidity(data);

    return 0;
}

/* =========================
 * SH1106
 * ========================= */
int sh1106_send_command(int fd, uint8_t cmd)
{
    uint8_t buf[2];
    buf[0] = 0x00; /* control byte: command */
    buf[1] = cmd;

    if (write(fd, buf, 2) != 2)
        return -1;

    return 0;
}

int sh1106_send_data(int fd, uint8_t data)
{
    uint8_t buf[2];
    buf[0] = 0x40; /* control byte: data */
    buf[1] = data;

    if (write(fd, buf, 2) != 2)
        return -1;

    return 0;
}

int sh1106_init(int fd)
{
    if (sh1106_send_command(fd, 0xAE) != 0)
        return -1; /* display off */
    if (sh1106_send_command(fd, 0xD5) != 0)
        return -1; /* display clock divide ratio / oscillator frequency */
    if (sh1106_send_command(fd, 0x80) != 0)
        return -1;
    if (sh1106_send_command(fd, 0xA8) != 0)
        return -1; /* multiplex ratio */
    if (sh1106_send_command(fd, 0x3F) != 0)
        return -1;
    if (sh1106_send_command(fd, 0xD3) != 0)
        return -1; /* display offset */
    if (sh1106_send_command(fd, 0x00) != 0)
        return -1;
    if (sh1106_send_command(fd, 0x40) != 0)
        return -1; /* display start line */
    if (sh1106_send_command(fd, 0xAD) != 0)
        return -1; /* charge pump */
    if (sh1106_send_command(fd, 0x8B) != 0)
        return -1;
    if (sh1106_send_command(fd, 0xA1) != 0)
        return -1; /* segment remap */
    if (sh1106_send_command(fd, 0xC8) != 0)
        return -1; /* COM scan direction */
    if (sh1106_send_command(fd, 0xDA) != 0)
        return -1; /* COM pins hardware configuration */
    if (sh1106_send_command(fd, 0x12) != 0)
        return -1;
    if (sh1106_send_command(fd, 0x81) != 0)
        return -1; /* contrast control */
    if (sh1106_send_command(fd, 0xBF) != 0)
        return -1;
    if (sh1106_send_command(fd, 0xD9) != 0)
        return -1; /* pre-charge period */
    if (sh1106_send_command(fd, 0x22) != 0)
        return -1;
    if (sh1106_send_command(fd, 0xDB) != 0)
        return -1; /* VCOMH deselect level */
    if (sh1106_send_command(fd, 0x40) != 0)
        return -1;
    if (sh1106_send_command(fd, 0x32) != 0)
        return -1; /* set Vpp */
    if (sh1106_send_command(fd, 0xA6) != 0)
        return -1; /* normal display */
    return 0;
}

int sh1106_set_page(int fd, uint8_t page)
{
    return sh1106_send_command(fd, 0xB0 + page);
}

int sh1106_set_column(int fd, uint8_t col)
{
    uint8_t low = col & 0x0F;
    uint8_t high = 0x10 | ((col >> 4) & 0x0F);

    if (sh1106_send_command(fd, low) != 0)
        return -1;
    if (sh1106_send_command(fd, high) != 0)
        return -1;

    return 0;
}

int sh1106_fill(int fd, uint8_t pattern)
{
    for (uint8_t page = 0; page < 8; page++)
    {
        if (sh1106_set_page(fd, page) != 0)
            return -1;

        if (sh1106_set_column(fd, 2) != 0)
            return -1;

        for (int col = 0; col < 128; col++)
        {
            if (sh1106_send_data(fd, pattern) != 0)
                return -1;
        }
    }

    return 0;
}

int sh1106_clear(int fd)
{
    return sh1106_fill(fd, 0x00);
}

int sh1106_set_cursor(int fd, uint8_t page, uint8_t col)
{
    if (sh1106_set_page(fd, page) != 0)
        return -1;
    if (sh1106_set_column(fd, col) != 0)
        return -1;
    return 0;
}

int sh1106_poweroff(int fd)
{
    if (sh1106_send_command(fd, 0xAE) != 0)
        return -1; /* display off */
}

int get_font_index(char c)
{
    switch (c)
    {
    case ' ':
        return 0;
    case '%':
        return 1;
    case '.':
        return 2;
    case '0':
        return 3;
    case '1':
        return 4;
    case '2':
        return 5;
    case '3':
        return 6;
    case '4':
        return 7;
    case '5':
        return 8;
    case '6':
        return 9;
    case '7':
        return 10;
    case '8':
        return 11;
    case '9':
        return 12;
    case ':':
        return 13;
    case 'C':
        return 14;
    case 'H':
        return 15;
    case 'T':
        return 16;
    case 'e':
        return 17;
    case 'i':
        return 18;
    case 'm':
        return 19;
    case 'p':
        return 20;
    case 'u':
        return 21;
    default:
        return 0;
    }
}

int sh1106_draw_char(int fd, char c)
{
    int idx = get_font_index(c);

    for (int i = 0; i < 5; i++)
    {
        if (sh1106_send_data(fd, font[idx][i]) != 0)
            return -1;
    }

    if (sh1106_send_data(fd, 0x00) != 0) /* 字元間空一欄 */
        return -1;

    return 0;
}

int sh1106_draw_string(int fd, const char *str)
{
    while (*str)
    {
        if (sh1106_draw_char(fd, *str) != 0)
            return -1;
        str++;
    }
    return 0;
}

/* =========================
 * Main
 * ========================= */
int main(void)
{
    int fd_aht20, fd_oled;
    float temperature, humidity;
    char line1[32];
    char line2[32];

    fd_aht20 = open(I2C_DEVICE, O_RDWR);
    if (fd_aht20 < 0)
    {
        perror("open aht20");
        return 1;
    }

    fd_oled = open(I2C_DEVICE, O_RDWR);
    if (fd_oled < 0)
    {
        perror("open oled");
        close(fd_aht20);
        return 1;
    }

    if (ioctl(fd_aht20, I2C_SLAVE, AHT20_ADDR) < 0)
    {
        perror("ioctl aht20");
        close(fd_aht20);
        close(fd_oled);
        return 1;
    }

    if (ioctl(fd_oled, I2C_SLAVE, SH1106_ADDR) < 0)
    {
        perror("ioctl oled");
        close(fd_aht20);
        close(fd_oled);
        return 1;
    }

    if (sh1106_init(fd_oled) != 0)
    {
        fprintf(stderr, "sh1106_init failed\n");
        close(fd_aht20);
        close(fd_oled);
        return 1;
    }

    if (sh1106_clear(fd_oled) != 0)
    {
        fprintf(stderr, "sh1106_clear failed\n");
        close(fd_aht20);
        close(fd_oled);
        return 1;
    }

    if (sh1106_send_command(fd_oled, 0xAF) != 0)
    {
        fprintf(stderr, "display on failed\n");
        close(fd_aht20);
        close(fd_oled);
        return 1;
    }

    while (1)
    {
        if (aht20_read_temperature_humidity(fd_aht20, &temperature, &humidity) != 0)
        {
            fprintf(stderr, "read AHT20 failed\n");
            sleep(1);
            continue;
        }

        snprintf(line1, sizeof(line1), "Temp: %.2f C", temperature);
        snprintf(line2, sizeof(line2), "Humi: %.2f %%", humidity);

        if (sh1106_clear(fd_oled) != 0)
        {
            fprintf(stderr, "sh1106_clear failed\n");
            break;
        }

        if (sh1106_set_cursor(fd_oled, 0, 2) != 0)
        {
            fprintf(stderr, "set cursor line1 failed\n");
            break;
        }

        if (sh1106_draw_string(fd_oled, line1) != 0)
        {
            fprintf(stderr, "draw line1 failed\n");
            break;
        }

        if (sh1106_set_cursor(fd_oled, 2, 2) != 0)
        {
            fprintf(stderr, "set cursor line2 failed\n");
            break;
        }

        if (sh1106_draw_string(fd_oled, line2) != 0)
        {
            fprintf(stderr, "draw line2 failed\n");
            break;
        }

        sleep(10);

    }

        close(fd_aht20);
    close(fd_oled);
    return 0;
}
