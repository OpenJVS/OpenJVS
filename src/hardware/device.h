#ifndef DEVICE_H_
#define DEVICE_H_

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include <time.h>
#include <stdarg.h>
#include <sys/ioctl.h>
#include <linux/serial.h>

#define IN 0
#define OUT 1

#define LOW 0
#define HIGH 1

int initDevice(char *devicePath, int senseLineType, int senseLinePin);
int closeDevice();
int readBytes(unsigned char *buffer, int amount);
int writeBytes(unsigned char *buffer, int amount);
int setSenseLine(int state);
int setupGPIO(int pin);
int setGPIODirection(int pin, int dir);
int readGPIO(int pin);
int setSerialAttributes(int fd, int myBaud);

#endif // DEVICE_H_
