#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>

int main() {
    const char *device = "/dev/i2c-1";
    int addr = 0x38;
    int fd;

    fd = open(device, O_RDWR);
    if (fd < 0) {
        perror("open");
        return 1;
    }

    if (ioctl(fd, I2C_SLAVE, addr) < 0) {
        perror("ioctl");
        close(fd);
        return 1;
    }

    // 初始化: write_i2c_block_data(addr, 0x71, [0x00])
    uint8_t init_cmd[2] = {0x71, 0x00};
    if (write(fd, init_cmd, 2) != 2) {
        perror("write init");
        close(fd);
        return 1;
    }
    usleep(100000);  // 0.1 秒

    // 觸發測量: write_i2c_block_data(addr, 0xAC, [0x33, 0x00])
    uint8_t measure_cmd[3] = {0xAC, 0x33, 0x00};
    if (write(fd, measure_cmd, 3) != 3) {
        perror("write measure");
        close(fd);
        return 1;
    }
    usleep(100000);  // 0.1 秒

    // 先寫入要讀的 register = 0x00
    uint8_t reg = 0x00;
    if (write(fd, &reg, 1) != 1) {
        perror("write reg");
        close(fd);
        return 1;
    }

    // 讀 6 bytes
    uint8_t data[6];
    if (read(fd, data, 6) != 6) {
        perror("read");
        close(fd);
        return 1;
    }

    // 轉溫度
    uint32_t raw_temp = ((data[3] & 0x0F) << 16) | (data[4] << 8) | data[5];
    float temp = (raw_temp / 1048576.0f) * 200.0f - 50.0f;

    printf("Temperature: %.2f °C\n", temp);

    close(fd);
    return 0;
}
