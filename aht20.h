#ifndef AHT20_H
#define AHT20_H

int aht20_init(int fd);
int aht20_read_temperature_humidity(int fd, float *temp, float *humi);

#endif
