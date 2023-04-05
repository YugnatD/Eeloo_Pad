#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "navball.h"
#include "image.h"

int main(int argc, char *argv[])
{
  printf("HELLO WORLD \n");
  // open the texture file
  imageRGB texture;
  imageRGB navballImage;
  openPPM(&texture, "NavBall_Texture.ppm");
  // alocate the navball image
  createImageRGB(&navballImage, 256, 256);
  generateNavBall(&texture, &navballImage, 0.0, 0.0, 0.0);
  // write back the ppm file for test purpose
  // savePPM(&texture, "NavBall_Texture2.ppm");
  // create the navball
  // free the texture
  freeImageRGB(&texture);
  return 0;
}
