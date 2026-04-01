#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>

#define I2C_DEVICE "/dev/i2c-1"
#define AHT20_ADDR 0x38

int main(void) {
    int fd;
    uint8_t status_cmd = 0x71;
    uint8_t measure_cmd[3] = {0xAC, 0x33, 0x00};
    uint8_t status;
    uint8_t data[7];

    // 1. 開啟 I2C device
    fd = open(I2C_DEVICE, O_RDWR);
    if (fd < 0) {
        perror("open");
        return 1;
    }

    // 2. 設定 slave address
    if (ioctl(fd, I2C_SLAVE, AHT20_ADDR) < 0) {
        perror("ioctl");
        close(fd);
        return 1;
    }

    // 上電後等待
    usleep(500000);  // 0.5 秒

    // 3. 讀 1 byte status
    if (write(fd, &status_cmd, 1) != 1) {
        perror("write status cmd");
        close(fd);
        return 1;
    }

    if (read(fd, &status, 1) != 1) {
        perror("read status");
        close(fd);
        return 1;
    }

    // 4. 檢查初始化狀態
    if ((status & 0x18) != 0x18) {
        fprintf(stderr, "Initialization error, status = 0x%02X\n", status);
        close(fd);
        return 1;
    }

    // 5. 送量測命令 0xAC 0x33 0x00
    if (write(fd, measure_cmd, 3) != 3) {
        perror("write measure cmd");
        close(fd);
        return 1;
    }

    usleep(100000);  // 0.1 秒

    // 6. 再送 0x71，準備讀資料
    if (write(fd, &status_cmd, 1) != 1) {
        perror("write read-data cmd");
        close(fd);
        return 1;
    }

    // 7. 讀 7 bytes
    if (read(fd, data, 7) != 7) {
        perror("read data");
        close(fd);
        return 1;
    }

    // 8. 解析溫度
    uint32_t Traw = ((data[3] & 0x0F) << 16) | (data[4] << 8) | data[5];
    float temperature = 200.0f * Traw / (1 << 20) - 50.0f;

    // 9. 解析濕度
    uint32_t Hraw = ((data[3] & 0xF0) >> 4) | (data[1] << 12) | (data[2] << 4);
    float humidity = 100.0f * Hraw / (1 << 20);

    printf("Temperature: %.2f °C\n", temperature);
    printf("Humidity: %.2f %%\n", humidity);

    close(fd);
    return 0;
}
