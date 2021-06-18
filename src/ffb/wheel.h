#ifndef WHEEL_H_
#define WHEEL_H_

int initWheel(int fd);
int setCentering(int value);
int setGain(int value);
int setForce(double force);
int setUncentering(int value);

#endif // WHEEL_H_
