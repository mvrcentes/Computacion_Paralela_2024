#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <cuda.h>
#include <string.h>
#include <vector>
#include "pgm.h"
#include <opencv2/opencv.hpp>

const int degreeInc = 2;
const int degreeBins = 180 / degreeInc;
const int rBins = 100;
const double radInc = degreeInc * M_PI / 180;

// Estructura para almacenar los parámetros de las líneas detectadas
struct Line {
    double r;
    double theta;
};

// Función en CPU para la Transformada de Hough
void CPU_HoughTran(unsigned char *pic, int w, int h, int **acc) {
    double rMax = sqrt(1.0 * w * w + 1.0 * h * h) / 2.0;
    *acc = new int[rBins * degreeBins];
    memset(*acc, 0, sizeof(int) * rBins * degreeBins);
    int xCent = w / 2;
    int yCent = h / 2;
    double rScale = (2.0 * rMax) / rBins;

    // Precomputar valores de theta
    double *thetaValues = new double[degreeBins];
    double theta = 0.0;
    for (int tIdx = 0; tIdx < degreeBins; tIdx++) {
        thetaValues[tIdx] = theta;
        theta += radInc;
    }

    for (int i = 0; i < w; i++) {
        for (int j = 0; j < h; j++) {
            int idx = j * w + i;
            if (pic[idx] > 0) {
                int xCoord = i - xCent;
                int yCoord = yCent - j;
                for (int tIdx = 0; tIdx < degreeBins; tIdx++) {
                    double r = xCoord * cos(thetaValues[tIdx]) + yCoord * sin(thetaValues[tIdx]);
                    int rIdx = (int)((r + rMax) / rScale + 0.5);
                    if (rIdx >= 0 && rIdx < rBins) {
                        (*acc)[rIdx * degreeBins + tIdx]++;
                    }
                }
            }
        }
    }
    delete[] thetaValues;
}

// Kernel de GPU para la Transformada de Hough (usando solo memoria global)
__global__ void GPU_HoughTran(unsigned char *pic, int w, int h, int *acc, double rMax, double rScale, double *d_Cos, double *d_Sin) {
    int gloID = blockIdx.x * blockDim.x + threadIdx.x;
    if (gloID >= w * h) return;

    int xCent = w / 2;
    int yCent = h / 2;

    int xCoord = gloID % w - xCent;
    int yCoord = yCent - gloID / w;

    if (pic[gloID] > 0) {
        for (int tIdx = 0; tIdx < degreeBins; tIdx++) {
            double r = xCoord * d_Cos[tIdx] + yCoord * d_Sin[tIdx];
            int rIdx = (int)((r + rMax) / rScale + 0.5);
            if (rIdx >= 0 && rIdx < rBins) {
                atomicAdd(acc + (rIdx * degreeBins + tIdx), 1);
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

    if (fabs(sinT) > 0.5) {
        for (int x = 0; x < w; x++) {
            double y = (r - (x - xCent) * cosT) / sinT;
            int yInt = yCent - (int)(y + 0.5);
            if (yInt >= 0 && yInt < h) {
                int idx = yInt * w + x;
                image[3 * idx] = 255;     // R
                image[3 * idx + 1] = 0;   // G
                image[3 * idx + 2] = 0;   // B
            }
        }
    } else {
        for (int y = 0; y < h; y++) {
            double x = (r - (yCent - y) * sinT) / cosT;
            int xInt = (int)(x + xCent + 0.5);
            if (xInt >= 0 && xInt < w) {
                int idx = y * w + xInt;
                image[3 * idx] = 255;     // R
                image[3 * idx + 1] = 0;   // G
                image[3 * idx + 2] = 0;   // B
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

    PGMImage inImg(argv[1]);
    int w = inImg.x_dim;
    int h = inImg.y_dim;

    // CPU Hough Transform
    int *cpuht;
    CPU_HoughTran(inImg.pixels, w, h, &cpuht);

    // Precompute sine and cosine tables
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

    double rMax = sqrt(1.0 * w * w + 1.0 * h * h) / 2.0;
    double rScale = (2.0 * rMax) / rBins;

    // Allocate and copy sine and cosine tables to device
    double *d_Cos, *d_Sin;
    cudaMalloc((void **)&d_Cos, sizeof(double) * degreeBins);
    cudaMalloc((void **)&d_Sin, sizeof(double) * degreeBins);
    cudaMemcpy(d_Cos, pcCos, sizeof(double) * degreeBins, cudaMemcpyHostToDevice);
    cudaMemcpy(d_Sin, pcSin, sizeof(double) * degreeBins, cudaMemcpyHostToDevice);

    unsigned char *d_in;
    int *d_hough;
    cudaMalloc((void **)&d_in, sizeof(unsigned char) * w * h);
    cudaMalloc((void **)&d_hough, sizeof(int) * degreeBins * rBins);
    cudaMemcpy(d_in, inImg.pixels, sizeof(unsigned char) * w * h, cudaMemcpyHostToDevice);
    cudaMemset(d_hough, 0, sizeof(int) * degreeBins * rBins);

    int threadsPerBlock = 256;
    int blockNum = (w * h + threadsPerBlock - 1) / threadsPerBlock;

    // Medición de tiempo en GPU
    cudaEvent_t start, stop;
    cudaEventCreate(&start);
    cudaEventCreate(&stop);
    cudaEventRecord(start);

    GPU_HoughTran<<<blockNum, threadsPerBlock>>>(d_in, w, h, d_hough, rMax, rScale, d_Cos, d_Sin);

    cudaEventRecord(stop);
    cudaEventSynchronize(stop);
    float elapsedTime;
    cudaEventElapsedTime(&elapsedTime, start, stop);
    printf("Tiempo en GPU: %f ms\n", elapsedTime);

    int *h_hough = (int *)malloc(degreeBins * rBins * sizeof(int));
    cudaMemcpy(h_hough, d_hough, sizeof(int) * degreeBins * rBins, cudaMemcpyDeviceToHost);

    cudaFree(d_in);
    cudaFree(d_hough);
    cudaFree(d_Cos);
    cudaFree(d_Sin);

    // Detección de líneas y umbral
    double sum = 0.0, sumSq = 0.0;
    int total = degreeBins * rBins;
    for (int i = 0; i < total; i++) {
        sum += h_hough[i];
        sumSq += h_hough[i] * h_hough[i];
    }
    double mean = sum / total;
    double variance = (sumSq / total) - (mean * mean);
    double threshold = mean + 3 * sqrt(variance);

    std::vector<Line> lines;
    for (int rIdx = 0; rIdx < rBins; rIdx++) {
        for (int tIdx = 0; tIdx < degreeBins; tIdx++) {
            int idx = rIdx * degreeBins + tIdx;
            if (h_hough[idx] > threshold) {
                double r = rIdx * rScale - rMax;
                double theta = thetaValues[tIdx];
                lines.push_back({r, theta});
            }
        }
    }

    // Crear imagen con líneas detectadas
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

    // Guardar resultados
    savePPM("output_global.ppm", resultImage, w, h);
    printf("Imagen con líneas guardada en 'output_global.ppm'\n");

    free(h_hough);
    delete[] cpuht;
    free(pcCos);
    free(pcSin);
    free(thetaValues);
    free(resultImage);

    return 0;
}
