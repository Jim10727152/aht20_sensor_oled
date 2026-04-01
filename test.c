#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>
#include <stdint.h>

int read_register(int fd, uint8_t reg);
int write_register(int fd, uint8_t reg, uint8_t val);


int read_register(int fd, uint8_t reg) {
    // tell device the reg before read
    // the passing parameter 1 is one byte
    if(write(fd, &reg, 1) != 1) {
        return -1;
    } // if

    // then read
    uint8_t data;
    if(read(fd, &data, 1) != 1) {
        return -1;
    } // if

    return data;
} // read_register()

int write_register(int fd, uint8_t reg, uint8_t val) {
    uint8_t buffer[2];
    buffer[0] = reg;
    buffer[1] = value;

    if (write(fd, buffer, 2) != 2) {
        return -1;
    }

    return 0;

} // write_register()

int main() {
    int addr = 0x38;
    int fd = open("/dev/i2c-1", O_RDWR);   // 1. 打開裝置
    ioctl(fd, I2C_SLAVE, addr);            // 2. 指定 slave device
    int temp = read_register(fd, 0x00);               // 3. 使用 fd
    printf("temp :%d\n", temp);
    close(fd);
    return 0;
}
