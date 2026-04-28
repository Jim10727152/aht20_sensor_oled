#define _DEFAULT_SOURCE

#include "aht20.h"
#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <unistd.h>
#include <sys/types.h>

/* AHT20 command */
#define AHT20_CMD_STATUS 0x71
#define AHT20_CMD_MEASURE 0xAC
#define AHT20_ARG_MEASURE1 0x33
#define AHT20_ARG_MEASURE2 0x00

/* AHT20 status bit */
#define AHT20_STATUS_BUSY 0x80
#define AHT20_STATUS_CALIBRATED 0x08

/* AHT20 config */
#define AHT20_DATA_LEN 6
#define AHT20_MAX_RETRY 10
#define AHT20_MEASURE_DELAY_US 80000

static int aht20_read_status(int fd, uint8_t *status);
static int aht20_trigger_measurement(int fd);
static int aht20_wait_until_ready(int fd);
static int aht20_read_data(int fd, uint8_t *data, size_t len);
static float aht20_parse_temperature(const uint8_t *data);
static float aht20_parse_humidity(const uint8_t *data);

int aht20_init(int fd)
{
    uint8_t status;

    if (fd < 0)
        return -1;

    if (aht20_read_status(fd, &status) != 0)
        return -1;

    /*
     * Bit 3 indicates whether the sensor is calibrated.
     * If this bit is not set, the sensor may need initialization.
     *
     * For this simple project, we only check the status.
     * A more complete driver can send the initialization command here.
     */
    if ((status & AHT20_STATUS_CALIBRATED) == 0)
        return -1;

    return 0;
}

int aht20_read_temperature_humidity(int fd, float *temperature, float *humidity)
{
    uint8_t data[AHT20_DATA_LEN];

    if (fd < 0 || temperature == NULL || humidity == NULL)
        return -1;

    if (aht20_trigger_measurement(fd) != 0)
        return -1;

    if (aht20_wait_until_ready(fd) != 0)
        return -1;

    if (aht20_read_data(fd, data, AHT20_DATA_LEN) != 0)
        return -1;

    *humidity = aht20_parse_humidity(data);
    *temperature = aht20_parse_temperature(data);

    return 0;
}

static int aht20_read_status(int fd, uint8_t *status)
{
    uint8_t cmd = AHT20_CMD_STATUS;

    if (fd < 0 || status == NULL)
        return -1;

    if (write(fd, &cmd, 1) != 1)
        return -1;

    if (read(fd, status, 1) != 1)
        return -1;

    return 0;
}

static int aht20_trigger_measurement(int fd)
{
    uint8_t cmd[3] = {
        AHT20_CMD_MEASURE,
        AHT20_ARG_MEASURE1,
        AHT20_ARG_MEASURE2};

    if (fd < 0)
        return -1;

    if (write(fd, cmd, sizeof(cmd)) != (ssize_t)sizeof(cmd))
        return -1;

    return 0;
}

static int aht20_wait_until_ready(int fd)
{
    uint8_t status;

    if (fd < 0)
        return -1;

    for (int retry = 0; retry < AHT20_MAX_RETRY; retry++)
    {
        usleep(AHT20_MEASURE_DELAY_US);

        if (aht20_read_status(fd, &status) != 0)
            return -1;

        if ((status & AHT20_STATUS_BUSY) == 0)
            return 0;
    }

    return -1;
}

static int aht20_read_data(int fd, uint8_t *data, size_t len)
{
    if (fd < 0 || data == NULL)
        return -1;

    if (len != AHT20_DATA_LEN)
        return -1;

    if (read(fd, data, len) != (ssize_t)len)
        return -1;

    return 0;
}

static float aht20_parse_temperature(const uint8_t *data)
{
    uint32_t raw_temperature;

    if (data == NULL)
        return 0.0f;

    raw_temperature =
        ((uint32_t)(data[3] & 0x0F) << 16) |
        ((uint32_t)data[4] << 8) |
        ((uint32_t)data[5]);

    return ((float)raw_temperature / 1048576.0f) * 200.0f - 50.0f;
}

static float aht20_parse_humidity(const uint8_t *data)
{
    uint32_t raw_humidity;

    if (data == NULL)
        return 0.0f;

    raw_humidity =
        ((uint32_t)data[1] << 12) |
        ((uint32_t)data[2] << 4) |
        ((uint32_t)(data[3] & 0xF0) >> 4);

    return ((float)raw_humidity / 1048576.0f) * 100.0f;
}
