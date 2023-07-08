#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include "navball.h"
#include "textureMap.h"

extern textureMap_t defaultTextureMap;

// int main(int argc, char *argv[])
int main()
{
  printf("HELLO WORLD \n");
  // open the texture file
  navballImage_t navballImage;
  // textureMap_t texture;
  // openTextureMap(&texture, "NavBall_Texture.ppm");
  // convert 45 deg to rad
  float pitch = 45.0 * M_PI / 180.0;
  float roll = 30.0 * M_PI / 180.0;
  float yaw = 10.0 * M_PI / 180.0;
  // generateNavBall(&texture, &navballImage, pitch, roll, yaw);
  generateNavBall(&defaultTextureMap, &navballImage, pitch, roll, yaw);
  // save the navball
  savePPM(&navballImage, "NavBall.ppm");
  return 0;
}
