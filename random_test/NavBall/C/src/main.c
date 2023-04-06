#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include "navball.h"
#include "image.h"

// int main(int argc, char *argv[])
int main()
{
  printf("HELLO WORLD \n");
  // open the texture file
  imageRGB texture;
  imageRGB navballImage;
  openPPM(&texture, "NavBall_Texture.ppm");
  // alocate the navball image
  createImageRGB(&navballImage, 16, 16);
  // convert 45 deg to rad
  float pitch = 45.0 * M_PI / 180.0;
  float roll = 90.0 * M_PI / 180.0;
  generateNavBall(&texture, &navballImage, pitch, roll, 0.0);
  // write back the ppm file for test purpose
  // savePPM(&texture, "NavBall_Texture2.ppm");
  // create the navball
  // free the texture
  freeImageRGB(&texture);
  return 0;
}
