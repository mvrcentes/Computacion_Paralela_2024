#include <SDL2/SDL.h>
#include <iostream>

const int WINDOW_WIDTH = 800;
const int WINDOW_HEIGHT = 600;

int main()
{
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
    while (SDL_PollEvent(&event))
    {
      if (event.type == SDL_QUIT)
      {
        running = false;
      }
    }

    // Limpiar la pantalla
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);

    // Aquí se pueden añadir llamadas de dibujo

    // Presentar lo dibujado
    SDL_RenderPresent(renderer);
  }

  // Limpiar recursos
  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
  SDL_Quit();

  return 0;
}
