CC = gcc
CFLAGS = -Wall -Wextra -Werror -std=c11
TARGET = sensor_oled

OBJS = main.o aht20.o sh1106.o

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS)

main.o: main.c aht20.h sh1106.h
	$(CC) $(CFLAGS) -c main.c

aht20.o: aht20.c aht20.h
	$(CC) $(CFLAGS) -c aht20.c

sh1106.o: sh1106.c sh1106.h
	$(CC) $(CFLAGS) -c sh1106.c

clean:
	rm -f $(OBJS) $(TARGET)
