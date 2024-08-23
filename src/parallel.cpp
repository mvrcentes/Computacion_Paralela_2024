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
  omp_set_num_threads(4); // Control de hilos: usar 4 hilos para la ejecución paralela

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
  uint32_t startTime = SDL_GetTicks(), frameCount = 0;

  while (running)
  {
    while (SDL_PollEvent(&event))
    {
      if (event.type == SDL_QUIT)
      {
        running = false;
      }
    }

#pragma omp parallel for
    for (int i = 0; i < numCircles; i++)
    {
      float newX = circles[i].x + circles[i].dx;
      float newY = circles[i].y + circles[i].dy;
      if (newX - circles[i].radius < 0 || newX + circles[i].radius > WINDOW_WIDTH)
      {
        circles[i].dx *= -1;
      }
      if (newY - circles[i].radius < 0 || newY + circles[i].radius > WINDOW_HEIGHT)
      {
        circles[i].dy *= -1;
      }
      circles[i].x = newX;
      circles[i].y = newY;
    }

    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);
    for (const auto &circle : circles)
    {
      filledCircleRGBA(renderer, circle.x, circle.y, circle.radius, circle.color.r, circle.color.g, circle.color.b, circle.color.a);
    }
    SDL_RenderPresent(renderer);

    frameCount++;
    if (SDL_GetTicks() - startTime >= 1000)
    {
      char title[100];
      snprintf(title, 100, "Screensaver - FPS: %d", frameCount);
      SDL_SetWindowTitle(window, title);
      frameCount = 0;
      startTime = SDL_GetTicks();
    }

    SDL_Delay(16); // Targeting approximately 60 FPS
  }

  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
  SDL_Quit();

  return 0;
}
