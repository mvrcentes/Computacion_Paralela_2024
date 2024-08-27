#include <SDL2/SDL.h>
#include <SDL2/SDL2_gfxPrimitives.h>
#include <iostream>
#include <vector>
#include <cstdlib>
#include <omp.h>
#include <cmath>
#include <string>

const int WINDOW_WIDTH = 800;
const int WINDOW_HEIGHT = 600;

struct Circle {
    float x, y;
    float dx, dy;
    int radius;
    SDL_Color color;
    bool collided;
};

// Función para calcular la distancia entre dos puntos
float distance(float x1, float y1, float x2, float y2) {
    return std::sqrt((x2 - x1) * (x2 - x1) + (y2 - y1) * (y2 - y1));
}

// Función para generar un color aleatorio
SDL_Color getRandomColor() {
    return { Uint8(rand() % 256), Uint8(rand() % 256), Uint8(rand() % 256), 255 };
}

// Función para manejar colisiones entre círculos
void handleCollision(Circle &a, Circle &b) {
    float dist = distance(a.x, a.y, b.x, b.y);
    if (dist < a.radius + b.radius) {
        if (a.radius > 10) a.radius -= 1;
        if (b.radius < 50) b.radius += 1;
        // Encontrar los vectores de colisión
        float nx = (b.x - a.x) / dist;
        float ny = (b.y - a.y) / dist;

        // Proyectar las velocidades en el vector de colisión
        float p = 2 * (a.dx * nx + a.dy * ny - b.dx * nx - b.dy * ny) / 2;

        // Actualizar las velocidades de los círculos
        a.dx -= p * nx;
        a.dy -= p * ny;
        b.dx += p * nx;
        b.dy += p * ny;

        // Mantener la velocidad dentro de un rango específico
        float speedLimit = 0.5; // Límite superior de velocidad
        float aSpeed = std::sqrt(a.dx * a.dx + a.dy * a.dy);
        float bSpeed = std::sqrt(b.dx * b.dx + b.dy * b.dy);

        if (aSpeed > speedLimit) {
            float scale = speedLimit / aSpeed;
            a.dx *= scale;
            a.dy *= scale;
        }

        if (bSpeed > speedLimit) {
            float scale = speedLimit / bSpeed;
            b.dx *= scale;
            b.dy *= scale;
        }

        // Separar los círculos para evitar solapamiento
        float overlap = 0.5f * (a.radius + b.radius - dist);
        a.x -= overlap * nx;
        a.y -= overlap * ny;
        b.x += overlap * nx;
        b.y += overlap * ny;

        // Cambiar los colores de los círculos después de la colisión
        #pragma omp critical  // Sección crítica para evitar condiciones de carrera
        {
            a.color = getRandomColor();
            b.color = getRandomColor();
        }
    }
}


void applyForces(std::vector<Circle> &circles) {
    float forceStrength = 0.01f; // Ajustar la intensidad de la fuerza
    
    #pragma omp parallel for
    for (int i = 0; i < circles.size(); i++) {
        for (int j = i + 1; j < circles.size(); j++) {
            float dist = distance(circles[i].x, circles[i].y, circles[j].x, circles[j].y);
            if (dist > 0 && dist < 200) { // Solo considerar círculos dentro de un cierto rango
                float force = forceStrength / dist;
                float nx = (circles[j].x - circles[i].x) / dist;
                float ny = (circles[j].y - circles[i].y) / dist;

                

                
                circles[i].dx -= force * nx;
                circles[i].dy -= force * ny;
                circles[j].dx += force * nx;
                circles[j].dy += force * ny;
            }
        }
    }
}

void attractToCenter(std::vector<Circle> &circles) {
    float centerX = WINDOW_WIDTH / 2.0f;
    float centerY = WINDOW_HEIGHT / 2.0f;
    float attractionStrength = 0.01f;

    #pragma omp parallel for
    for (int i = 0; i < circles.size(); i++) {
        float dist = distance(circles[i].x, circles[i].y, centerX, centerY);
        float nx = (centerX - circles[i].x) / dist;
        float ny = (centerY - circles[i].y) / dist;

        circles[i].dx += attractionStrength * nx;
        circles[i].dy += attractionStrength * ny;
    }
}

// Función para generar un número aleatorio entre 0 y 1
float randomFloat() {
    return static_cast<float>(rand()) / static_cast<float>(RAND_MAX);
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <number_of_circles>" << std::endl;
        return 1;
    }

    int numCircles = std::atoi(argv[1]);
    int numThreads = 8;  // Número de hilos a usar
    omp_set_num_threads(numThreads);

    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        std::cerr << "Error initializing SDL: " << SDL_GetError() << std::endl;
        return 1;
    }

    SDL_Window *window = SDL_CreateWindow("Parallel Screensaver", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_SHOWN);
    if (!window) {
        std::cerr << "Error creating window: " << SDL_GetError() << std::endl;
        SDL_Quit();
        return 1;
    }

    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer) {
        std::cerr << "Error creating renderer: " << SDL_GetError() << std::endl;
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    std::vector<Circle> circles(numCircles);

    #pragma omp parallel for
    for (int i = 0; i < numCircles; i++) {
        circles[i].x = rand() % WINDOW_WIDTH;
        circles[i].y = rand() % WINDOW_HEIGHT;
        circles[i].dx = (float(rand()) / RAND_MAX) * 1.5 + 0.5;  // Rango: 0.5 a 2.0
        circles[i].dy = (float(rand()) / RAND_MAX) * 1.5 + 0.5;  // Rango: 0.5 a 2.0
        circles[i].radius = (rand() % 20) + 10;
        circles[i].color = getRandomColor();
        circles[i].collided = false;
    }

    Uint64 startTick, endTick, currentTick;
    double deltaTime, fps;
    bool running = true;
    SDL_Event event;
    char fpsText[50];

    const double applyForcesInterval = 2.0;  // Intervalo de tiempo para la función applyForces (en segundos)
    const double attractToCenterInterval = 1.0;  // Intervalo de tiempo para la función attractToCenter (en segundos)
    double elapsed = 0.0;  // Tiempo transcurrido desde el último cambio

    // Establecer el tiempo inicial
    startTick = SDL_GetPerformanceCounter();
    
    // Indica qué función aplicar actualmente
    bool applyForcesEnabled = true;

    while (running) {
        currentTick = SDL_GetPerformanceCounter();
        deltaTime = (double)(currentTick - startTick) / SDL_GetPerformanceFrequency();
        
        // Cambiar función según el intervalo de tiempo
        if (elapsed >= (applyForcesEnabled ? applyForcesInterval : attractToCenterInterval)) {
            applyForcesEnabled = !applyForcesEnabled;  // Alternar la función
            elapsed = 0.0;  // Reiniciar el temporizador
        }

        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = false;
            }
        }

        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        #pragma omp parallel sections
    {
        #pragma omp section
        applyForces(circles);     // Interacción de atracción/repulsión
    }

        // Actualizar posiciones
        #pragma omp parallel for schedule(dynamic)
        for (int i = 0; i < numCircles; i++) {
            circles[i].x += circles[i].dx;
            circles[i].y += circles[i].dy;
            if (circles[i].x - circles[i].radius < 0 || circles[i].x + circles[i].radius > WINDOW_WIDTH) {
                circles[i].dx *= -1;
            }
            if (circles[i].y - circles[i].radius < 0 || circles[i].y + circles[i].radius > WINDOW_HEIGHT) {
                circles[i].dy *= -1;
            }
        }

        // Detectar y manejar colisiones
        #pragma omp parallel for schedule(dynamic)
        for (int i = 0; i < numCircles; i++) {
            for (int j = i + 1; j < numCircles; j++) {
                handleCollision(circles[i], circles[j]);
            }
        }

        // Renderizado secuencial
        for (const auto &circle : circles) {
            if (circle.radius > 0) { // Solo renderizar círculos con radio positivo
                filledCircleRGBA(renderer, circle.x, circle.y, circle.radius, circle.color.r, circle.color.g, circle.color.b, circle.color.a);
            }
        }

        // Actualizar el tiempo transcurrido
        elapsed += deltaTime;

        // Mostrar FPS
        sprintf(fpsText, "FPS: %.2f", 1.0 / deltaTime);
        stringRGBA(renderer, 10, 10, fpsText, 255, 255, 255, 255);

        SDL_RenderPresent(renderer);

        // Actualizar el tiempo de inicio
        startTick = currentTick;
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}