#ifndef FFB_H_
#define FFB_H_

#include "io.h"

int initFFB(int fd);
int setCentering(int value);
int setGain(int value);
int setForce(double force);
int setUncentering(int value);

int processJVSFFB(JVSState* state);

#endif // FFB_H_
