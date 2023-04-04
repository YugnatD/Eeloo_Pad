#ifndef NAVBALL_H
#define NAVBALL_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include <math.h>

#include "image.h"

void generateNavBall(imageRGB *texture, imageRGB *navballImage, float pitch, float roll, float yaw);

#endif
