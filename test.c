#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>

int main() {
    int file;
    int addr = 0x3c;

    file = open("/dev/i2c-1", O_RDWR);
    ioctl(file, I2C_SLAVE, addr);

    char reg = 0x00;
    write(file, &reg, 1);

    char data;
    read(file, &data, 1);

    printf("Data: %d\n", data);

    close(file);
    return 0;
}
