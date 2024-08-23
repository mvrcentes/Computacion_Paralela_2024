#include <SDL2/SDL.h>
#include <SDL2/SDL2_gfxPrimitives.h>
#include <iostream>
#include <vector>
#include <cstdlib>
#include <omp.h>

const int WINDOW_WIDTH = 800;
const int WINDOW_HEIGHT = 600;

struct Circle
{
  float x, y;
  float dx, dy;
  int radius;
  SDL_Color color;
};

int main(int argc, char *argv[])
{
  if (argc < 2)
  {
    std::cerr << "Usage: " << argv[0] << " <number_of_circles>" << std::endl;
    return 1;
  }

  int numCircles = std::atoi(argv[1]);
  if (numCircles <= 0)
  {
    std::cerr << "The number of circles should be a positive integer." << std::endl;
    return 1;
  }

  if (SDL_Init(SDL_INIT_VIDEO) != 0)
  {
    std::cerr << "Error initializing SDL: " << SDL_GetError() << std::endl;
    return 1;
  }

  SDL_Window *window = SDL_CreateWindow("Screensaver", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_SHOWN);
  if (!window)
  {
    std::cerr << "Error creating window: " << SDL_GetError() << std::endl;
    SDL_Quit();
    return 1;
  }

  SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
  if (!renderer)
  {
    std::cerr << "Error creating renderer: " << SDL_GetError() << std::endl;
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 1;
  }

  std::vector<Circle> circles(numCircles);
#pragma omp parallel for
  for (int i = 0; i < numCircles; i++)
  {
    circles[i].x = rand() % WINDOW_WIDTH;
    circles[i].y = rand() % WINDOW_HEIGHT;
    circles[i].dx = (rand() % 5) + 1;
    circles[i].dy = (rand() % 5) + 1;
    circles[i].radius = (rand() % 20) + 10;
    circles[i].color = {Uint8(rand() % 256), Uint8(rand() % 256), Uint8(rand() % 256), 255};
  }

  bool running = true;
  SDL_Event event;

  while (running)
  {
    while (SDL_PollEvent(&event))
    {
      if (event.type == SDL_QUIT)
      {
        running = false;
      }
    }

    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);

#pragma omp parallel for
    for (int i = 0; i < numCircles; i++)
    {
      circles[i].x += circles[i].dx;
      circles[i].y += circles[i].dy;
      if (circles[i].x - circles[i].radius < 0 || circles[i].x + circles[i].radius > WINDOW_WIDTH)
      {
        circles[i].dx *= -1;
      }
      if (circles[i].y - circles[i].radius < 0 || circles[i].y + circles[i].radius > WINDOW_HEIGHT)
      {
        circles[i].dy *= -1;
      }
      filledCircleRGBA(renderer, circles[i].x, circles[i].y, circles[i].radius, circles[i].color.r, circles[i].color.g, circles[i].color.b, circles[i].color.a);
    }

    SDL_RenderPresent(renderer);
  }

  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
  SDL_Quit();

  return 0;
}
