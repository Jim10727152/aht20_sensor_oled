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
