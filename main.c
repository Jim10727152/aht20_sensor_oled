#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>

#include "aht20.h"
#include "sh1106.h"

#define I2C_DEVICE   "/dev/i2c-1"
#define AHT20_ADDR   0x38
#define SH1106_ADDR  0x3C

#define UPDATE_INTERVAL_SEC 10

static int open_i2c_device(const char *device_path, int slave_addr)
{
    int fd;

    if (device_path == NULL)
        return -1;

    fd = open(device_path, O_RDWR);
    if (fd < 0) {
        perror("open i2c device");
        return -1;
    }

    if (ioctl(fd, I2C_SLAVE, slave_addr) < 0) {
        perror("ioctl I2C_SLAVE");
        close(fd);
        return -1;
    }

    return fd;
}

static int display_temperature_humidity(int fd_oled, float temperature, float humidity)
{
    char line1[32];
    char line2[32];

    if (fd_oled < 0)
        return -1;

    snprintf(line1, sizeof(line1), "Temp: %.2f C", temperature);
    snprintf(line2, sizeof(line2), "Humi: %.2f %%", humidity);

    if (sh1106_clear(fd_oled) != 0)
        return -1;

    if (sh1106_set_cursor(fd_oled, 0, 2) != 0)
        return -1;

    if (sh1106_draw_string(fd_oled, line1) != 0)
        return -1;

    if (sh1106_set_cursor(fd_oled, 2, 2) != 0)
        return -1;

    if (sh1106_draw_string(fd_oled, line2) != 0)
        return -1;

    return 0;
}

int main(void)
{
    int ret = EXIT_FAILURE;
    int fd_aht20 = -1;
    int fd_oled = -1;
    float temperature = 0.0f;
    float humidity = 0.0f;

    fd_aht20 = open_i2c_device(I2C_DEVICE, AHT20_ADDR);
    if (fd_aht20 < 0) {
        fprintf(stderr, "Failed to open AHT20 device\n");
        goto cleanup;
    }

    fd_oled = open_i2c_device(I2C_DEVICE, SH1106_ADDR);
    if (fd_oled < 0) {
        fprintf(stderr, "Failed to open SH1106 device\n");
        goto cleanup;
    }

    if (aht20_init(fd_aht20) != 0) {
        fprintf(stderr, "AHT20 initialization failed\n");
        goto cleanup;
    }

    if (sh1106_init(fd_oled) != 0) {
        fprintf(stderr, "SH1106 initialization failed\n");
        goto cleanup;
    }

    if (sh1106_clear(fd_oled) != 0) {
        fprintf(stderr, "SH1106 clear failed\n");
        goto cleanup;
    }

    while (1) {
        if (aht20_read_temperature_humidity(fd_aht20, &temperature, &humidity) != 0) {
            fprintf(stderr, "Failed to read temperature and humidity\n");
            sleep(1);
            continue;
        }

        if (display_temperature_humidity(fd_oled, temperature, humidity) != 0) {
            fprintf(stderr, "Failed to update OLED display\n");
            break;
        }

        sleep(UPDATE_INTERVAL_SEC);
    }

    ret = EXIT_SUCCESS;

cleanup:
    if (fd_oled >= 0) {
        sh1106_poweroff(fd_oled);
        close(fd_oled);
    }

    if (fd_aht20 >= 0) {
        close(fd_aht20);
    }

    return ret;
}
