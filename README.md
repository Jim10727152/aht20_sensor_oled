# Raspberry Pi Temperature and Humidity Sensor Reader (AHT20)

This project is a simple C program on Raspberry Pi for reading temperature and humidity data from an AHT20 sensor over the I2C interface.

The program communicates with the sensor through Linux's `/dev/i2c-1` device, sends measurement commands, reads raw sensor data, and converts the result into human-readable temperature and humidity values.


## reference

https://hackmd.io/xTgy64DCQUGVAlp3CVSeKA

## demo

![demo](images/demo_aht20_oled.jpg)
