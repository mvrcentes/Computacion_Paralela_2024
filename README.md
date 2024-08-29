
# Screensaver Project

This project is a screensaver developed in C++ using SDL2 for graphical rendering and OpenMP for parallelization. The screensaver features moving circles that interact with each other and the center of the window.

## Description

The screensaver simulates a series of circles moving across the window and responding to collisions between them. The circles change color and adjust in size upon collision. The simulation can be run in either sequential or parallel mode, with parallelization improving performance as the workload increases.

## Features

- **Circle Movement**: Circles move across the window with random velocities.
- **Collisions**: Circles detect and handle collisions with each other.
- **Center Attraction**: Circles are attracted towards the center of the window.
- **Rendering**: Uses SDL2 for graphical rendering of the circles.
- **Parallelization**: Utilizes OpenMP to parallelize the force and collision calculations.

## Requirements

- **SDL2**: Make sure you have SDL2 installed.
- **SDL2_gfx**: Additional library for graphic functions.
- **OpenMP**: For code parallelization.

## Installation

1. **Clone the repository**:
    ```bash
    git clone https://github.com/mvrcentes/Computacion_Paralela_2024.git
    cd screensaver-project
    ```

2. **Install dependencies**:
    - On Debian-based systems (like Ubuntu):
      ```bash
      sudo apt-get install libsdl2-dev libsdl2-gfx-dev
      ```
    - On macOS, use Homebrew:
      ```bash
      brew install sdl2 sdl2_gfx
      ```

3. **Install OpenMP** (if not already installed):
    - On Debian-based systems:
      ```bash
      sudo apt-get install libomp-dev
      ```
    - On macOS, OpenMP is usually included with Homebrew installations of GCC:
      ```bash
      brew install gcc
      ```

4. **Compile the project** with OpenMP:
    ```bash
    g++ parallel.cpp -o ./build/screensaver -lSDL2 -lSDL2_gfx -fopenmp
    ```
    ```bash
    g++ main.cpp -o ./build/sequential_screensaver -lSDL2 -lSDL2_gfx
    ```

## Usage

To run the screensaver, use the following command:

```bash
./build/screensaver <number_of_circles>
```

Where `<number_of_circles>` is the number of circles to display in the window.

## How It Works

- **Initialization**: The program initializes SDL2 and creates a window and renderer.
- **Circle Setup**: Circles are generated with random properties.
- **Simulation**: Alternates between applying attraction/repulsion forces and attracting circles towards the center of the window.
- **Rendering**: Circles are rendered in the window and updated based on their positions and velocities.

## Parallelization

The project uses OpenMP to parallelize the following aspects:
- Force calculations and updates.
- Collision detection and resolution.

To optimize performance with OpenMP, ensure that your compiler supports OpenMP and use the `-fopenmp` flag during compilation.

## Contribution

If you want to contribute to this project, please follow these steps:
1. Fork the repository.
2. Create a new branch (`git checkout -b feature-branch`).
3. Make your changes and commit (`git commit -am 'Add new feature'`).
4. Push the changes to your repository (`git push origin feature-branch`).
5. Create a pull request.



### Notes:
- **Installation**: Instructions for installing OpenMP are included.
- **Compilation**: The `-fopenmp` flag is specified for enabling OpenMP during compilation.
- **Parallelization**: Added details about the use of OpenMP in the project.

Feel free to adjust the repository URL, email address, and other specifics to match your actual project details.
