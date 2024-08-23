#include <SDL2/SDL.h>
#include <SDL2/SDL2_gfxPrimitives.h>
#include <iostream>
#include <vector>
#include <cstdlib>
#include <omp.h>
#include <string>

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
  omp_set_num_threads(4);

  if (SDL_Init(SDL_INIT_VIDEO) != 0)
  {
    std::cerr << "Error initializing SDL: " << SDL_GetError() << std::endl;
    return 1;
  }

  SDL_Window *window = SDL_CreateWindow("Parallel Screensaver", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_SHOWN);
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

  Uint64 startTick, endTick;
  double deltaTime, fps;
  bool running = true;
  SDL_Event event;
  char fpsText[50];

  while (running)
  {
    startTick = SDL_GetPerformanceCounter();

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
    }

    for (const auto &circle : circles)
    {
      filledCircleRGBA(renderer, circle.x, circle.y, circle.radius, circle.color.r, circle.color.g, circle.color.b, circle.color.a);
    }

    endTick = SDL_GetPerformanceCounter();
    deltaTime = (double)(endTick - startTick) / SDL_GetPerformanceFrequency();
    fps = 1.0 / deltaTime;
    sprintf(fpsText, "FPS: %.2f", fps);
    stringRGBA(renderer, 10, 10, fpsText, 255, 255, 255, 255);

    SDL_RenderPresent(renderer);
  }

  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
  SDL_Quit();

  return 0;
}