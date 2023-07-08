#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <SDL2/SDL.h>
#include "navball.h"
#include "textureMap.h"

extern textureMap_t defaultTextureMap;

// int main(int argc, char *argv[])
int main()
{
  SDL_Window* window = NULL;
  SDL_Renderer* renderer = NULL;
  SDL_Event event;
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

  SDL_Init(SDL_INIT_VIDEO);
  SDL_CreateWindowAndRenderer(SIZE_NAVBALL,SIZE_NAVBALL,SDL_WINDOW_SHOWN,&window,&renderer);
  SDL_RenderClear(renderer);
  int quit = 0;
  uint32_t pixelR = 0;
  uint32_t pixelG = 0;
  uint32_t pixelB = 0;
  while (!quit)
  {
    // measure time between generation
    clock_t start = clock();
    generateNavBall(&defaultTextureMap, &navballImage, pitch, roll, yaw);
    clock_t end = clock();
    float seconds = (float)(end - start) / CLOCKS_PER_SEC;
    printf("Time to generate: %f\n", seconds);
    pitch += 0.01;
    roll += 0.01;
    yaw += 0.01;
    SDL_RenderClear(renderer);    
    for(uint32_t li=0;li<SIZE_NAVBALL;li++)
    {
      for(uint32_t co=0;co<SIZE_NAVBALL;co++)
      {
        
        pixelR = navballImage.data[li][co][0];
        pixelG = navballImage.data[li][co][1];
        pixelB = navballImage.data[li][co][2];
        if(SDL_SetRenderDrawColor(renderer,pixelR,pixelG,pixelB,SDL_ALPHA_OPAQUE)!=0){printf("ERROR Renderer\n");}
          SDL_RenderDrawPoint(renderer,co,li);
        }
        if (SDL_PollEvent(&event)) {
          if (event.type == SDL_QUIT) {
            quit = 1;
            break;
          }
      }
    }
    SDL_RenderPresent(renderer);
  }
  return 0;
}
