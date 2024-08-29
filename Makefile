# Definición de variables
CC=clang++  # Usar clang++
CFLAGS=-std=c++11 -Wall -Xpreprocessor -fopenmp -I/opt/homebrew/include -I/opt/homebrew/Cellar/libomp/18.1.8/include
LDFLAGS=-L/opt/homebrew/lib -lSDL2 -lSDL2_gfx -L/opt/homebrew/Cellar/libomp/18.1.8/lib -lomp
SRC_DIR=src
BUILD_DIR=build

# Asegúrate de que el directorio build exista
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# Compilar el proyecto para main.cpp
screensaver_main: $(BUILD_DIR)/screensaver_main

$(BUILD_DIR)/screensaver_main: $(SRC_DIR)/main.cpp | $(BUILD_DIR)
	$(CC) $(CFLAGS) $< -o $@ $(LDFLAGS)

# Compilar el proyecto para parallel.cpp
screensaver_parallel: $(BUILD_DIR)/screensaver_parallel

$(BUILD_DIR)/screensaver_parallel: $(SRC_DIR)/parallel.cpp | $(BUILD_DIR)
	$(CC) $(CFLAGS) $< -o $@ $(LDFLAGS)

# Limpiar archivos compilados
clean:
	rm -f $(BUILD_DIR)/*

# Ejecutar los programas
run_main: screensaver_main
	./$(BUILD_DIR)/screensaver_main

run_parallel: screensaver_parallel
	./$(BUILD_DIR)/screensaver_parallel

# Objetivo por defecto
all: screensaver_main screensaver_parallel
