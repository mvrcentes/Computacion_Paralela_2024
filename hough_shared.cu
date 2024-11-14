#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <cuda.h>
#include <string.h>
#include <vector>
#include "pgm.h"              // Biblioteca personalizada para leer y procesar imágenes PGM
#include <opencv2/opencv.hpp> // Biblioteca OpenCV para guardar imágenes en diferentes formatos

// Configuración de la Transformada de Hough
const int degreeInc = 2;                       // Incremento en grados para el ángulo theta
const int degreeBins = 180 / degreeInc;        // Número de divisiones para theta (0 a 180 grados)
const int rBins = 100;                         // Número de divisiones para r (distancia perpendicular a la línea)
const double radInc = degreeInc * M_PI / 180;  // Incremento en radianes para theta

// Declaración de memoria constante para seno y coseno en la GPU
__constant__ double d_Cos[degreeBins];
__constant__ double d_Sin[degreeBins];

// Estructura para almacenar los parámetros de las líneas detectadas
struct Line {
    double r;       // Distancia desde el origen
    double theta;   // Ángulo de la línea en radianes
};

//*****************************************************************
// Función de Transformada de Hough en CPU
void CPU_HoughTran(unsigned char *pic, int w, int h, int **acc) {
    // Calcular el valor máximo de r (distancia diagonal de la imagen)
    double rMax = sqrt(1.0 * w * w + 1.0 * h * h) / 2.0;

    // Inicializar el acumulador en memoria con valores cero
    *acc = new int[rBins * degreeBins];
    memset(*acc, 0, sizeof(int) * rBins * degreeBins);

    // Calcular el centro de la imagen
    int xCent = w / 2;
    int yCent = h / 2;

    // Escala de r para ajustar los valores al rango de bins
    double rScale = (2.0 * rMax) / rBins;

    // Iterar sobre todos los píxeles de la imagen
    for (int i = 0; i < w; i++) {
        for (int j = 0; j < h; j++) {
            int idx = j * w + i; // Índice lineal del píxel
            if (pic[idx] > 0) { // Verificar si el píxel es parte del borde
                int xCoord = i - xCent;
                int yCoord = yCent - j;
                for (int tIdx = 0; tIdx < degreeBins; tIdx++) {
                    // Calcular r usando el valor precalculado de theta
                    double r = xCoord * cos(tIdx * radInc) + yCoord * sin(tIdx * radInc);
                    int rIdx = (int)((r + rMax) / rScale + 0.5);
                    // Aumentar el contador en el acumulador si rIdx es válido
                    if (rIdx >= 0 && rIdx < rBins) {
                        (*acc)[rIdx * degreeBins + tIdx]++;
                    }
                }
            }
        }
    }
}

// Kernel para Transformada de Hough en GPU usando memoria compartida
__global__ void GPU_HoughTran_Shared(unsigned char *pic, int w, int h, int *acc, double rMax, double rScale) {
    // Declarar un acumulador local en memoria compartida para cada bloque
    extern __shared__ int localAcc[]; 

    int gloID = blockIdx.x * blockDim.x + threadIdx.x; // Identificar el índice global del hilo
    int tIdx = threadIdx.x; // Índice dentro del bloque

    // Inicializar el acumulador local en memoria compartida
    if (tIdx < degreeBins) {
        for (int rIdx = 0; rIdx < rBins; rIdx++) {
            localAcc[rIdx * degreeBins + tIdx] = 0;
        }
    }
    __syncthreads();

    // Calcular el centro de la imagen
    int xCent = w / 2;
    int yCent = h / 2;

    // Procesar píxeles si el índice es válido y si el píxel pertenece al borde
    if (gloID < w * h && pic[gloID] > 0) {
        int xCoord = gloID % w - xCent;
        int yCoord = yCent - gloID / w;

        // Iterar sobre todos los ángulos de theta
        for (int tIdx = 0; tIdx < degreeBins; tIdx++) {
            double r = xCoord * d_Cos[tIdx] + yCoord * d_Sin[tIdx];
            int rIdx = (int)((r + rMax) / rScale + 0.5);
            if (rIdx >= 0 && rIdx < rBins) {
                atomicAdd(&localAcc[rIdx * degreeBins + tIdx], 1); // Usar operación atómica para acumulador
            }
        }
    }
    __syncthreads();

    // Transferir los datos del acumulador local al acumulador global en memoria compartida
    if (tIdx < degreeBins) {
        for (int rIdx = 0; rIdx < rBins; rIdx++) {
            atomicAdd(&acc[rIdx * degreeBins + tIdx], localAcc[rIdx * degreeBins + tIdx]);
        }
    }
}

// Función para dibujar una línea sobre la imagen en color
void drawLine(unsigned char *image, int w, int h, double r, double theta) {
    int xCent = w / 2;
    int yCent = h / 2;

    double cosT = cos(theta);
    double sinT = sin(theta);

    // Dibujar línea usando la ecuación y = (r - x * cos(theta)) / sin(theta)
    if (fabs(sinT) > 0.5) {
        for (int x = 0; x < w; x++) {
            double y = (r - (x - xCent) * cosT) / sinT;
            int yInt = yCent - (int)(y + 0.5);
            if (yInt >= 0 && yInt < h) {
                int idx = yInt * w + x;
                image[3 * idx] = 0;         // Componente R (rojo)
                image[3 * idx + 1] = 255;   // Componente G (verde)
                image[3 * idx + 2] = 255;   // Componente B (azul)
            }
        }
    } else {
        // Dibujar línea usando la ecuación x = (r - y * sin(theta)) / cos(theta)
        for (int y = 0; y < h; y++) {
            double x = (r - (yCent - y) * sinT) / cosT;
            int xInt = (int)(x + xCent + 0.5);
            if (xInt >= 0 && xInt < w) {
                int idx = y * w + xInt;
                image[3 * idx] = 0;         // Componente R (rojo)
                image[3 * idx + 1] = 255;   // Componente G (verde)
                image[3 * idx + 2] = 255;   // Componente B (azul)
            }
        }
    }
}

// Función para guardar la imagen en formato PPM (color)
void savePPM(const char *filename, unsigned char *image, int w, int h) {
    FILE *fp = fopen(filename, "wb");
    if (!fp) {
        printf("Error al abrir el archivo para escribir: %s\n", filename);
        return;
    }
    fprintf(fp, "P6\n%d %d\n255\n", w, h);
    fwrite(image, sizeof(unsigned char), w * h * 3, fp);
    fclose(fp);
}

int main(int argc, char **argv) {
    if (argc < 2) {
        printf("Uso: %s <imagen.pgm>\n", argv[0]);
        return -1;
    }

    // Leer imagen de entrada en formato PGM
    PGMImage inImg(argv[1]);
    int w = inImg.x_dim;
    int h = inImg.y_dim;

    // Realizar Transformada de Hough en CPU
    int *cpuht;
    CPU_HoughTran(inImg.pixels, w, h, &cpuht);

    // Preparar tablas de seno y coseno para GPU
    double *thetaValues = (double *)malloc(sizeof(double) * degreeBins);
    double *pcCos = (double *)malloc(sizeof(double) * degreeBins);
    double *pcSin = (double *)malloc(sizeof(double) * degreeBins);
    double theta = 0.0;
    for (int i = 0; i < degreeBins; i++) {
        thetaValues[i] = theta;
        pcCos[i] = cos(theta);
        pcSin[i] = sin(theta);
        theta += radInc;
    }

    double rMax = sqrt(1.0 * w * w + 1.0 * h * h) / 2.0; // Calcular r máximo
    double rScale = (2.0 * rMax) / rBins; // Calcular escala para los valores de r

    // Copiar valores de seno y coseno a memoria constante en GPU
    cudaMemcpyToSymbol(d_Cos, pcCos, sizeof(double) * degreeBins);
    cudaMemcpyToSymbol(d_Sin, pcSin, sizeof(double) * degreeBins);

    // Reservar memoria en GPU y copiar imagen de entrada
    unsigned char *d_in;
    int *d_hough;
    cudaMalloc((void **)&d_in, sizeof(unsigned char) * w * h);
    cudaMalloc((void **)&d_hough, sizeof(int) * degreeBins * rBins);
    cudaMemcpy(d_in, inImg.pixels, sizeof(unsigned char) * w * h, cudaMemcpyHostToDevice);
    cudaMemset(d_hough, 0, sizeof(int) * degreeBins * rBins);

    // Configuración para kernel en GPU
    int threadsPerBlock = 256;
    int blockNum = (w * h + threadsPerBlock - 1) / threadsPerBlock;
    size_t sharedMemSize = degreeBins * rBins * sizeof(int);

    // Medición de tiempo de GPU
    cudaEvent_t start, stop;
    cudaEventCreate(&start);
    cudaEventCreate(&stop);
    cudaEventRecord(start);

    // Ejecutar kernel en GPU
    GPU_HoughTran_Shared<<<blockNum, threadsPerBlock, sharedMemSize>>>(d_in, w, h, d_hough, rMax, rScale);

    cudaEventRecord(stop);
    cudaEventSynchronize(stop);
    float elapsedTime;
    cudaEventElapsedTime(&elapsedTime, start, stop);
    printf("Tiempo en GPU: %f ms\n", elapsedTime);

    int *h_hough = (int *)malloc(degreeBins * rBins * sizeof(int));
    cudaMemcpy(h_hough, d_hough, sizeof(int) * degreeBins * rBins, cudaMemcpyDeviceToHost);

    // Detección de líneas basada en el umbral
    double sum = 0.0, sumSq = 0.0;
    int total = degreeBins * rBins;
    for (int i = 0; i < total; i++) {
        sum += h_hough[i];
        sumSq += h_hough[i] * h_hough[i];
    }
    double mean = sum / total;
    double threshold = mean + 2.55 * sqrt((sumSq / total) - (mean * mean));

    std::vector<Line> lines;
    for (int rIdx = 0; rIdx < rBins; rIdx++) {
        for (int tIdx = 0; tIdx < degreeBins; tIdx++) {
            if (h_hough[rIdx * degreeBins + tIdx] > threshold) {
                lines.push_back({rIdx * rScale - rMax, thetaValues[tIdx]});
            }
        }
    }

    // Crear imagen de salida y dibujar las líneas detectadas
    unsigned char *resultImage = (unsigned char *)malloc(w * h * 3 * sizeof(unsigned char));
    for (int idx = 0; idx < w * h; idx++) {
        resultImage[3 * idx] = resultImage[3 * idx + 1] = resultImage[3 * idx + 2] = inImg.pixels[idx];
    }
    for (const auto &line : lines) {
        drawLine(resultImage, w, h, line.r, line.theta);
    }

    // Guardar imagen de salida en PPM y PNG
    savePPM("output_shared.ppm", resultImage, w, h);
    cv::imwrite("output_shared.png", cv::Mat(h, w, CV_8UC3, resultImage));

    // Liberar memoria
    free(h_hough);
    delete[] cpuht;
    free(pcCos);
    free(pcSin);
    free(thetaValues);
    free(resultImage);

    return 0;
}
