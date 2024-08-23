#include <SDL2/SDL.h>
#include <SDL2/SDL2_gfxPrimitives.h>
#include <iostream>
#include <vector>

const int WINDOW_WIDTH = 800;
const int WINDOW_HEIGHT = 600;

struct Circle
{
  float x, y;      // Posición
  float dx, dy;    // Velocidad en x e y
  int radius;      // Radio
  SDL_Color color; // Color
};

int main()
{

  std::vector<Circle> circles;
  for (int i = 0; i < 5; i++)
  { // Crear 5 círculos
    Circle c;
    c.x = rand() % WINDOW_WIDTH;
    c.y = rand() % WINDOW_HEIGHT;
    c.dx = (rand() % 5) + 1;
    c.dy = (rand() % 5) + 1;
    c.radius = (rand() % 20) + 10;
    c.color = {Uint8(rand() % 256), Uint8(rand() % 256), Uint8(rand() % 256), 255};
    circles.push_back(c);
  }
  // Inicializar SDL
  if (SDL_Init(SDL_INIT_VIDEO) != 0)
  {
    std::cerr << "Error al inicializar SDL: " << SDL_GetError() << std::endl;
    return 1;
  }

  // Crear ventana
  SDL_Window *window = SDL_CreateWindow("Screensaver",
                                        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                        WINDOW_WIDTH, WINDOW_HEIGHT,
                                        SDL_WINDOW_SHOWN);
  if (!window)
  {
    std::cerr << "Error al crear ventana: " << SDL_GetError() << std::endl;
    SDL_Quit();
    return 1;
  }

  // Crear renderizador
  SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
  if (!renderer)
  {
    std::cerr << "Error al crear renderizador: " << SDL_GetError() << std::endl;
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 1;
  }

  // Loop principal
  bool running = true;
  SDL_Event event;
  while (running)
  {
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer); // Mover esta línea al inicio del loop

    while (SDL_PollEvent(&event))
    {
      if (event.type == SDL_QUIT)
      {
        running = false;
      }
    }

    for (auto &c : circles)
    {
      // Mover el círculo
      c.x += c.dx;
      c.y += c.dy;

      // Rebotar en los bordes de la ventana
      if (c.x - c.radius < 0 || c.x + c.radius > WINDOW_WIDTH)
        c.dx *= -1;
      if (c.y - c.radius < 0 || c.y + c.radius > WINDOW_HEIGHT)
        c.dy *= -1;

      // Dibujar el círculo
      filledCircleRGBA(renderer, c.x, c.y, c.radius, c.color.r, c.color.g, c.color.b, c.color.a);
    }

    // Presentar lo dibujado
    SDL_RenderPresent(renderer);
  }

  // Limpiar recursos
  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
  SDL_Quit();

  return 0;
}
