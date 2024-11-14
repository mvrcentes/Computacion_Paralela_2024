#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <cuda.h>
#include <string.h>
#include <vector>
#include "pgm.h"              // Biblioteca personalizada para leer imágenes PGM
#include <opencv2/opencv.hpp> // Biblioteca OpenCV para guardar imágenes en diferentes formatos

// Configuración de la Transformada de Hough
const int degreeInc = 2; // Incremento en grados para el ángulo theta
const int degreeBins = 180 / degreeInc; // Número de divisiones en theta
const int rBins = 100; // Número de divisiones en r (distancia perpendicular a la línea)
const double radInc = degreeInc * M_PI / 180; // Incremento en radianes para theta

// Declaración de variables constantes para seno y coseno en GPU
__constant__ double d_Cos[degreeBins];
__constant__ double d_Sin[degreeBins];

// Estructura para almacenar los parámetros de las líneas detectadas
struct Line {
    double r;       // Distancia desde el origen
    double theta;   // Ángulo de la línea en radianes
};

//*****************************************************************
// Función CPU para la Transformada de Hough
void CPU_HoughTran(unsigned char *pic, int w, int h, int **acc) {
    // Calcular el valor máximo de r (hipotenusa del rectángulo que cubre la imagen)
    double rMax = sqrt(1.0 * w * w + 1.0 * h * h) / 2.0;

    // Inicializar el acumulador en memoria
    *acc = new int[rBins * degreeBins];
    memset(*acc, 0, sizeof(int) * rBins * degreeBins);

    // Calcular el centro de la imagen
    int xCent = w / 2;
    int yCent = h / 2;

    // Escala de r para ajustar los valores al rango de bins
    double rScale = (2.0 * rMax) / rBins;

    // Precalcular los valores de theta para optimizar cálculos
    double *thetaValues = new double[degreeBins];
    double theta = 0.0;
    for (int tIdx = 0; tIdx < degreeBins; tIdx++) {
        thetaValues[tIdx] = theta;
        theta += radInc;
    }

    // Iterar sobre todos los píxeles de la imagen
    for (int i = 0; i < w; i++) {
        for (int j = 0; j < h; j++) {
            int idx = j * w + i;
            if (pic[idx] > 0) { // Verificar si el píxel es parte del borde
                int xCoord = i - xCent;
                int yCoord = yCent - j;
                for (int tIdx = 0; tIdx < degreeBins; tIdx++) {
                    // Calcular r usando el valor precalculado de theta
                    double r = xCoord * cos(thetaValues[tIdx]) + yCoord * sin(thetaValues[tIdx]);
                    int rIdx = (int)((r + rMax) / rScale + 0.5);
                    // Aumentar el contador en el acumulador si rIdx es válido
                    if (rIdx >= 0 && rIdx < rBins) {
                        (*acc)[rIdx * degreeBins + tIdx]++;
                    }
                }
            }
        }
    }
    delete[] thetaValues;
}

// GPU kernel para la Transformada de Hough
__global__ void GPU_HoughTran(unsigned char *pic, int w, int h, int *acc, double rMax, double rScale) {
    // Identificar el hilo global
    int gloID = blockIdx.x * blockDim.x + threadIdx.x;
    if (gloID >= w * h) return;

    int xCent = w / 2;
    int yCent = h / 2;

    // Calcular coordenadas relativas al centro de la imagen
    int xCoord = gloID % w - xCent;
    int yCoord = yCent - gloID / w;

    // Si el píxel es parte del borde, procesarlo
    if (pic[gloID] > 0) {
        for (int tIdx = 0; tIdx < degreeBins; tIdx++) {
            double r = xCoord * d_Cos[tIdx] + yCoord * d_Sin[tIdx];
            int rIdx = (int)((r + rMax) / rScale + 0.5);
            if (rIdx >= 0 && rIdx < rBins) {
                atomicAdd(acc + (rIdx * degreeBins + tIdx), 1); // Actualización atómica en el acumulador
            }
        }
    }
}

// Función para dibujar una línea sobre la imagen en color
void drawLine(unsigned char *image, int w, int h, double r, double theta) {
    int xCent = w / 2;
    int yCent = h / 2;

    double cosT = cos(theta);
    double sinT = sin(theta);

    // Dibujar línea basada en la ecuación y = (r - x * cos(theta)) / sin(theta)
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
    } else { // Dibujar línea basada en la ecuación x = (r - y * sin(theta)) / cos(theta)
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

    PGMImage inImg(argv[1]); // Leer imagen de entrada en formato PGM
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
    double rScale = (2.0 * rMax) / rBins; // Escala para ajustar valores de r

    // Copiar valores precalculados de seno y coseno a memoria constante en GPU
    cudaMemcpyToSymbol(d_Cos, pcCos, sizeof(double) * degreeBins);
    cudaMemcpyToSymbol(d_Sin, pcSin, sizeof(double) * degreeBins);

    // Asignar memoria en GPU para imagen y acumulador
    unsigned char *d_in;
    int *d_hough;
    cudaMalloc((void **)&d_in, sizeof(unsigned char) * w * h);
    cudaMalloc((void **)&d_hough, sizeof(int) * degreeBins * rBins);
    cudaMemcpy(d_in, inImg.pixels, sizeof(unsigned char) * w * h, cudaMemcpyHostToDevice);
    cudaMemset(d_hough, 0, sizeof(int) * degreeBins * rBins);

    // Configuración para el kernel
    int threadsPerBlock = 256;
    int blockNum = (w * h + threadsPerBlock - 1) / threadsPerBlock;
    cudaEvent_t start, stop;
    cudaEventCreate(&start);
    cudaEventCreate(&stop);
    cudaEventRecord(start);

    // Ejecutar Transformada de Hough en GPU
    GPU_HoughTran<<<blockNum, threadsPerBlock>>>(d_in, w, h, d_hough, rMax, rScale);

    // Sincronizar y medir tiempo de GPU
    cudaEventRecord(stop);
    cudaEventSynchronize(stop);
    float elapsedTime;
    cudaEventElapsedTime(&elapsedTime, start, stop);
    printf("Tiempo de GPU: %f ms\n", elapsedTime);

    // Copiar el acumulador de GPU a CPU
    int *h_hough = (int *)malloc(degreeBins * rBins * sizeof(int));
    cudaMemcpy(h_hough, d_hough, sizeof(int) * degreeBins * rBins, cudaMemcpyDeviceToHost);

    // Liberar memoria de GPU
    cudaFree(d_in);
    cudaFree(d_hough);

    // Detectar líneas en el acumulador usando umbral
    double sum = 0.0, sumSq = 0.0;
    int total = degreeBins * rBins;
    for (int i = 0; i < total; i++) {
        sum += h_hough[i];
        sumSq += h_hough[i] * h_hough[i];
    }
    double mean = sum / total;
    double variance = (sumSq / total) - (mean * mean);
    double stddev = sqrt(variance);
    double threshold = (mean + 2 * stddev) * 1.20;

    std::vector<Line> lines;
    for (int rIdx = 0; rIdx < rBins; rIdx++) {
        for (int tIdx = 0; tIdx < degreeBins; tIdx++) {
            int idx = rIdx * degreeBins + tIdx;
            if (h_hough[idx] > threshold) {
                double r = rIdx * rScale - rMax;
                double theta = thetaValues[tIdx];
                Line line = {r, theta};
                lines.push_back(line);
            }
        }
    }
    printf("Cantidad de líneas detectadas: %lu\n", lines.size());

    // Crear imagen de salida con líneas dibujadas
    unsigned char *resultImage = (unsigned char *)malloc(w * h * 3 * sizeof(unsigned char));
    for (int idx = 0; idx < w * h; idx++) {
        unsigned char pixel = inImg.pixels[idx];
        resultImage[3 * idx] = pixel;
        resultImage[3 * idx + 1] = pixel;
        resultImage[3 * idx + 2] = pixel;
    }
    for (const auto &line : lines) {
        drawLine(resultImage, w, h, line.r, line.theta);
    }

    // Guardar imagen en formato PNG y PPM
    cv::Mat imgMat(h, w, CV_8UC3, resultImage);
    cv::imwrite("constant_output.png", imgMat);
    printf("Imagen 'constant_output.png' creada\n");

    savePPM("constant_output.ppm", resultImage, w, h);
    printf("Imagen 'constant_output.ppm' creada\n");

    // Liberar memoria en CPU
    free(h_hough);
    delete[] cpuht;
    free(pcCos);
    free(pcSin);
    free(thetaValues);
    free(resultImage);

    return 0;
}
