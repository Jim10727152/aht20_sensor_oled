# 小實作 - 量測溫度

## Reference
[dht20 datasheet](https://aqicn.org/air/sensor/spec/asair-dht20.pdf)
[我的github](https://github.com/Jim10727152/pi_sensor/tree/main)

## I2C 讀資料
https://github.com/Jim10727152/pi_sensor/blob/main/test.c
write → 指定 register
read  → 讀資料

**I2C protocol**
START → addr → reg → RESTART → addr → read
## 0. 準備
windows 在 cmd 中，SSH 連進去樹莓派終端機

```
ssh jim@192.XXX.X.XXX
```

準備麵包版、公對母杜邦線、dht20溫溼度感測器

參考下圖接樹梅派的腳位
![image](https://hackmd.io/_uploads/SJi2TgcoZg.png)


![image](https://hackmd.io/_uploads/HJFTCx9oWg.png)

連接如下：


| DHT20 | Pi | 
| -------- | -------- | 
| 1     | 1     | 
| 2     | 3     | 
| 3     | 5     | 
| 4     | 6     | 


## 1. 開啟 Raspberry Pi I2C
terminal下指令
```
$ sudo raspi-config
```
選
```
Interface Options → I2C → Enable
```
terminal下指令
```
sudo apt update
sudo apt install -y i2c-tools
```
掃 I2C devices
```
i2cdetect -y 1
```
會看到如下表示正確，0x38 就是感測器的 device address
![image](https://hackmd.io/_uploads/SklaXZcobg.png)

## 2. 執行程式碼，量測溫度
python程式碼
``` python
import smbus2
import time

bus = smbus2.SMBus(1)
addr = 0x38

# 初始化
bus.write_i2c_block_data(addr, 0x71, [0x00])
time.sleep(0.1)

# 觸發測量
bus.write_i2c_block_data(addr, 0xAC, [0x33, 0x00])
time.sleep(0.1)

# 讀資料
data = bus.read_i2c_block_data(addr, 0x00, 6)

# 轉溫度
raw_temp = ((data[3] & 0x0F) << 16) | (data[4] << 8) | data[5]
temp = (raw_temp / 1048576.0) * 200 - 50

print(f"Temperature: {temp:.2f} °C")
```

![image](https://hackmd.io/_uploads/rJA7om5obx.png)

## 3. 延伸 使用 C 撰寫上述程式碼

``` c
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
```
