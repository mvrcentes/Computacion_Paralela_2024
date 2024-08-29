#include <SDL2/SDL.h>
#include <SDL2/SDL2_gfxPrimitives.h>
#include <iostream>
#include <vector>
#include <cstdlib>
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

        float nx = (b.x - a.x) / dist;
        float ny = (b.y - a.y) / dist;

        float p = 2 * (a.dx * nx + a.dy * ny - b.dx * nx - b.dy * ny) / 2;

        a.dx -= p * nx;
        a.dy -= p * ny;
        b.dx += p * nx;
        b.dy += p * ny;

        float speedLimit = 0.5;
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

        float overlap = 0.5f * (a.radius + b.radius - dist);
        a.x -= overlap * nx;
        a.y -= overlap * ny;
        b.x += overlap * nx;
        b.y += overlap * ny;

        a.color = getRandomColor();
        b.color = getRandomColor();
    }
}

void applyForces(std::vector<Circle> &circles) {
    float forceStrength = 0.01f;

    for (int i = 0; i < circles.size(); i++) {
        Circle& ci = circles[i];
        for (int j = i + 1; j < circles.size(); j++) {
            Circle& cj = circles[j];
            float dist = distance(ci.x, ci.y, cj.x, cj.y);
            if (dist > 0 && dist < 200) {
                float force = forceStrength / dist;
                float nx = (cj.x - ci.x) / dist;
                float ny = (cj.y - ci.y) / dist;

                ci.dx -= force * nx;
                ci.dy -= force * ny;

                cj.dx += force * nx;
                cj.dy += force * ny;
            }
        }
    }
}

void attractToCenter(std::vector<Circle> &circles) {
    float centerX = WINDOW_WIDTH / 2.0f;
    float centerY = WINDOW_HEIGHT / 2.0f;
    float attractionStrength = 0.01f;

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

    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        std::cerr << "Error initializing SDL: " << SDL_GetError() << std::endl;
        return 1;
    }

    SDL_Window *window = SDL_CreateWindow("Sequential Screensaver", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_SHOWN);
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

    startTick = SDL_GetPerformanceCounter();
    bool applyForcesEnabled = true;

    while (running) {
        currentTick = SDL_GetPerformanceCounter();
        deltaTime = (double)(currentTick - startTick) / SDL_GetPerformanceFrequency();
        
        if (elapsed >= (applyForcesEnabled ? applyForcesInterval : attractToCenterInterval)) {
            applyForcesEnabled = !applyForcesEnabled;
            elapsed = 0.0;
        }

        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = false;
            }
        }

        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        applyForces(circles);

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

        for (int i = 0; i < numCircles; i++) {
            for (int j = i + 1; j < numCircles; j++) {
                handleCollision(circles[i], circles[j]);
            }
        }

        for (const auto &circle : circles) {
            if (circle.radius > 0) {
                filledCircleRGBA(renderer, circle.x, circle.y, circle.radius, circle.color.r, circle.color.g, circle.color.b, circle.color.a);
            }
        }

        elapsed += deltaTime;

        if (deltaTime > 0) {
            sprintf(fpsText, "FPS: %.2f", 1.0 / deltaTime);
        } else {
            sprintf(fpsText, "FPS: --");
        }

        stringRGBA(renderer, 10, 10, fpsText, 255, 255, 255, 255);

        SDL_RenderPresent(renderer);

        startTick = currentTick;
    }

    SDL_Delay(16);

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
