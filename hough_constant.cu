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

__constant__ double d_Cos[degreeBins];
__constant__ double d_Sin[degreeBins];

// Estructura para almacenar los parámetros de las líneas detectadas
struct Line {
    double r;
    double theta;
};

//*****************************************************************
// Función CPU para la Transformada de Hough
void CPU_HoughTran(unsigned char *pic, int w, int h, int **acc) {
    double rMax = sqrt(1.0 * w * w + 1.0 * h * h) / 2.0;
    *acc = new int[rBins * degreeBins];
    memset(*acc, 0, sizeof(int) * rBins * degreeBins);
    int xCent = w / 2;
    int yCent = h / 2;
    double rScale = (2.0 * rMax) / rBins;

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

// GPU kernel para la Transformada de Hough
__global__ void GPU_HoughTran(unsigned char *pic, int w, int h, int *acc, double rMax, double rScale) {
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
                image[3 * idx] = 0;     // R
                image[3 * idx + 1] = 255;   // G
                image[3 * idx + 2] = 255;   // B
            }
        }
    } else {
        for (int y = 0; y < h; y++) {
            double x = (r - (yCent - y) * sinT) / cosT;
            int xInt = (int)(x + xCent + 0.5);
            if (xInt >= 0 && xInt < w) {
                int idx = y * w + xInt;
                image[3 * idx] = 0;     // R
                image[3 * idx + 1] = 255;   // G
                image[3 * idx + 2] = 255;   // B
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

    int *cpuht;
    int w = inImg.x_dim;
    int h = inImg.y_dim;

    CPU_HoughTran(inImg.pixels, w, h, &cpuht);

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

    cudaMemcpyToSymbol(d_Cos, pcCos, sizeof(double) * degreeBins);
    cudaMemcpyToSymbol(d_Sin, pcSin, sizeof(double) * degreeBins);

    unsigned char *d_in;
    int *d_hough;

    cudaMalloc((void **)&d_in, sizeof(unsigned char) * w * h);
    cudaMalloc((void **)&d_hough, sizeof(int) * degreeBins * rBins);
    cudaMemcpy(d_in, inImg.pixels, sizeof(unsigned char) * w * h, cudaMemcpyHostToDevice);
    cudaMemset(d_hough, 0, sizeof(int) * degreeBins * rBins);

    int threadsPerBlock = 256;
    int blockNum = (w * h + threadsPerBlock - 1) / threadsPerBlock;
    cudaEvent_t start, stop;
    cudaEventCreate(&start);
    cudaEventCreate(&stop);
    cudaEventRecord(start);
    GPU_HoughTran<<<blockNum, threadsPerBlock>>>(d_in, w, h, d_hough, rMax, rScale);
    cudaEventRecord(stop);
    cudaEventSynchronize(stop);
    float elapsedTime;
    cudaEventElapsedTime(&elapsedTime, start, stop);
    printf("Tiempo de GPU: %f ms\n", elapsedTime);

    int *h_hough = (int *)malloc(degreeBins * rBins * sizeof(int));
    cudaMemcpy(h_hough, d_hough, sizeof(int) * degreeBins * rBins, cudaMemcpyDeviceToHost);

    cudaFree(d_in);
    cudaFree(d_hough);

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

    unsigned char *resultImage = (unsigned char *)malloc(w * h * 3 * sizeof(unsigned char));
    for (int idx = 0; idx < w * h; idx++) {
        unsigned char pixel = inImg.pixels[idx];
        resultImage[3 * idx] = pixel;
        resultImage[3 * idx + 1] = pixel;
        resultImage[3 * idx + 2] = pixel;
    }

    for (size_t idx = 0; idx < lines.size(); idx++) {
        drawLine(resultImage, w, h, lines[idx].r, lines[idx].theta);
    }

    savePPM("constant_output.ppm", resultImage, w, h);
    printf("Imagen 'output_hough.ppm' creada\n");

    cv::Mat imgMat(h, w, CV_8UC3, resultImage);
    cv::imwrite("constant_output.png", imgMat);
    printf("Imagen 'output_hough.png' creada\n");

    free(h_hough);
    delete[] cpuht;
    free(pcCos);
    free(pcSin);
    free(thetaValues);
    free(resultImage);

    return 0;
}
