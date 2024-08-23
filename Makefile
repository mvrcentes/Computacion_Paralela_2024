# Definición de variables
CC=clang++  # Usar clang++
CFLAGS=-std=c++11 -Wall -Xpreprocessor -fopenmp -I/opt/homebrew/include -I/opt/homebrew/Cellar/libomp/18.1.8/include
LDFLAGS=-L/opt/homebrew/lib -lSDL2 -lSDL2_gfx -L/opt/homebrew/Cellar/libomp/18.1.8/lib -lomp
SRC_DIR=src
BUILD_DIR=build

# Compilar el proyecto
all: $(BUILD_DIR) screensaver

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

screensaver:
	$(CC) $(CFLAGS) $(SRC_DIR)/main.cpp -o $(BUILD_DIR)/screensaver $(LDFLAGS)

# Limpiar archivos compilados
clean:
	rm -rf $(BUILD_DIR)/*

# Ejecutar el programa
run: screensaver
	./$(BUILD_DIR)/screensaver
