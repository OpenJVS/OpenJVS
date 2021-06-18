#include "hardware/device.h"
#include "console/debug.h"

#define TIMEOUT_SELECT 200

int serialIO = -1;
int localSenseLinePin = 12;
int localSenseLineType = 0;

int setSerialAttributes(int fd, int myBaud);
int setupGPIO(int pin);
int setGPIODirection(int pin, int dir);
int writeGPIO(int pin, int value);

int initDevice(char *devicePath, int senseLineType, int senseLinePin)
{
  if ((serialIO = open(devicePath, O_RDWR | O_NOCTTY | O_SYNC | O_NDELAY)) < 0)
    return 0;

  /* Setup the serial connection */
  setSerialAttributes(serialIO, B115200);

  /* Copy variables over from config */
  localSenseLineType = senseLineType;
  localSenseLinePin = senseLinePin;

  /* Setup the GPIO pins */
  if (localSenseLineType && setupGPIO(localSenseLinePin) == -1)
    debug(0, "Sense line pin %d not available\n", senseLinePin);

  /* Setup the GPIO pins initial state */
  switch (senseLineType)
  {
  case 0:
    debug(1, "Debug: No sense line set\n");
    break;
  case 1:
    debug(1, "Debug: Float/Sync sense line set\n");
    setGPIODirection(senseLinePin, IN);
    break;
  case 2:
    debug(1, "Debug: Complex sense line set\n");
    setGPIODirection(senseLinePin, OUT);
    break;
  default:
    debug(0, "Debug: Invalid sense line type set\n");
    break;
  }

  /* Initially float the sense line */
  setSenseLine(0);

  return 1;
}

int closeDevice()
{
  tcflush(serialIO, TCIOFLUSH);
  return close(serialIO) == 0;
}

int readBytes(unsigned char *buffer, int amount)
{
  fd_set fd_serial;
  struct timeval tv;

  FD_ZERO(&fd_serial);
  FD_SET(serialIO, &fd_serial);

  tv.tv_sec = 0;
  tv.tv_usec = TIMEOUT_SELECT * 1000;

  int filesReadyToRead = select(serialIO + 1, &fd_serial, NULL, NULL, &tv);

  if (filesReadyToRead < 1)
    return -1;

  if (!FD_ISSET(serialIO, &fd_serial))
    return -1;

  return read(serialIO, buffer, amount);
}

int writeBytes(unsigned char *buffer, int amount)
{
  return write(serialIO, buffer, amount);
}

/* Sets the configuration of the serial port */
int setSerialAttributes(int fd, int myBaud)
{
  struct termios options;
  int status;
  tcgetattr(fd, &options);

  cfmakeraw(&options);
  cfsetispeed(&options, myBaud);
  cfsetospeed(&options, myBaud);

  options.c_cflag |= (CLOCAL | CREAD);
  options.c_cflag &= ~PARENB;
  options.c_cflag &= ~CSTOPB;
  options.c_cflag &= ~CSIZE;
  options.c_cflag |= CS8;
  options.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
  options.c_oflag &= ~OPOST;

  options.c_cc[VMIN] = 0;
  options.c_cc[VTIME] = 0; // One seconds (10 deciseconds)

  tcsetattr(fd, TCSANOW, &options);

  ioctl(fd, TIOCMGET, &status);

  status |= TIOCM_DTR;
  status |= TIOCM_RTS;

  ioctl(fd, TIOCMSET, &status);

  usleep(100 * 1000); // 10mS

  struct serial_struct serial_settings;

  ioctl(fd, TIOCGSERIAL, &serial_settings);

  serial_settings.flags |= ASYNC_LOW_LATENCY;
  ioctl(fd, TIOCSSERIAL, &serial_settings);

  tcflush(serialIO, TCIOFLUSH);
  usleep(100 * 1000); // Required to make flush work, for some reason

  return 0;
}

int setupGPIO(int pin)
{
  char buffer[3];
  ssize_t bytesWritten;
  int fd;

  if ((fd = open("/sys/class/gpio/export", O_WRONLY)) == -1)
    return 0;

  bytesWritten = snprintf(buffer, 3, "%d", pin);
  if (write(fd, buffer, bytesWritten) != bytesWritten)
    return 0;

  close(fd);
  return 1;
}

int setGPIODirection(int pin, int dir)
{
  static const char s_directions_str[] = "in\0out";

  char path[35];
  int fd;

  snprintf(path, 35, "/sys/class/gpio/gpio%d/direction", pin);
  if ((fd = open(path, O_WRONLY)) == -1)
    return 0;

  int length = IN == dir ? 2 : 3;
  if (write(fd, &s_directions_str[IN == dir ? 0 : 3], length) != length)
    return 0;

  close(fd);
  return 1;
}

int writeGPIO(int pin, int value)
{
  static const char stringValues[] = "01";

  char path[100];
  int fd;

  snprintf(path, 100, "/sys/class/gpio/gpio%d/value", pin);
  if ((fd = open(path, O_WRONLY)) == -1)
    return 0;

  if (write(fd, &stringValues[LOW == value ? 0 : 1], 1) != 1)
    return 0;

  close(fd);
  return 1;
}

int readGPIO(int pin)
{
  char path[100];
  char value_str[3];
  int fd;

  snprintf(path, 100, "/sys/class/gpio/gpio%d/value", pin);
  if ((fd = open(path, O_RDONLY)) == -1)
    return -1;

  if (read(fd, value_str, 3) == -1)
    return -1;

  close(fd);
  return (atoi(value_str));
}

int setSenseLine(int state)
{
  if (localSenseLineType == 0)
    return 1;

  switch (localSenseLineType)
  {
  /* Normal Float Style */
  case 1:
  {
    if (!state)
    {
      if (!setGPIODirection(localSenseLinePin, IN))
      {
        debug(1, "Warning: Failed to float sense line %d\n", localSenseLinePin);
        return 0;
      }
    }
    else
    {
      if (!setGPIODirection(localSenseLinePin, OUT) || !writeGPIO(localSenseLinePin, 0))
      {
        debug(1, "Warning: Failed to sink sense line %d\n", localSenseLinePin);
        return 0;
      }
    }
  }
  break;

  /* Switch Style */
  case 2:
  {
    if (!state)
    {
      if (!writeGPIO(localSenseLinePin, 0))
      {
        printf("Warning: Failed to set sense line to 1 %d\n", localSenseLinePin);
        return 0;
      }
    }
    else
    {
      if (!writeGPIO(localSenseLinePin, 1))
      {
        printf("Warning: Failed to sink sense line %d\n", localSenseLinePin);
        return 0;
      }
    }
  }
  break;

  default:
    debug(0, "Invalid sense line type set\n");
    break;
  }

  return 1;
}
