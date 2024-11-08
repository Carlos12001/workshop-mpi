// sobel_openmp.c
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <omp.h>

// Estructuras para manejar el encabezado BMP
#pragma pack(push, 1)
typedef struct {
    unsigned short type;
    unsigned int size;
    unsigned short reserved1;
    unsigned short reserved2;
    unsigned int offset;
} BMPHeader;

typedef struct {
    unsigned int size;
    int width;
    int height;
    unsigned short planes;
    unsigned short bitCount;
    unsigned int compression;
    unsigned int imageSize;
    int xPixelsPerMeter;
    int yPixelsPerMeter;
    unsigned int colorsUsed;
    unsigned int colorsImportant;
} BMPInfoHeader;
#pragma pack(pop)

// Función para aplicar el filtro Sobel con OpenMP
void sobel_filter_omp(unsigned char *data, unsigned char *output, int width, int height) {
    int Gx[3][3] = { {-1, 0, 1}, {-2, 0, 2}, {-1, 0, 1} };
    int Gy[3][3] = { {-1, -2, -1}, {0, 0, 0}, {1, 2, 1} };

    int rowSize = ((width * 3 + 3) / 4) * 4;
    unsigned char *grayData = malloc(width * height);

    // Convertir a escala de grises
    #pragma omp parallel for
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            int pos = y * rowSize + x * 3;
            unsigned char gray = (data[pos] + data[pos + 1] + data[pos + 2]) / 3;
            grayData[y * width + x] = gray;
        }
    }

    // Aplicar el filtro Sobel
    #pragma omp parallel for
    for (int y = 1; y < height - 1; y++) {
        for (int x = 1; x < width - 1; x++) {
            int gx = 0, gy = 0;
            for (int i = -1; i <= 1; i++) {
                for (int j = -1; j <= 1; j++) {
                    int pixel = grayData[(y + i) * width + (x + j)];
                    gx += Gx[i + 1][j + 1] * pixel;
                    gy += Gy[i + 1][j + 1] * pixel;
                }
            }
            int magnitude = (int)sqrt(gx * gx + gy * gy);
            if (magnitude > 255) magnitude = 255;
            int pos = y * rowSize + x * 3;
            output[pos] = magnitude;
            output[pos + 1] = magnitude;
            output[pos + 2] = magnitude;
        }
    }

    free(grayData);
}

int main() {
    BMPHeader header;
    BMPInfoHeader infoHeader;
    unsigned char *data, *output;
    char input_filename[50];
    char output_filename[50];

    for (int img = 1; img <= 5; img++) {
        sprintf(input_filename, "images/%d.bmp", img);
        FILE *file = fopen(input_filename, "rb");
        if (!file) {
            printf("No se pudo abrir la imagen %s\n", input_filename);
            continue;
        }

        fread(&header, sizeof(BMPHeader), 1, file);
        fread(&infoHeader, sizeof(BMPInfoHeader), 1, file);

        int width = infoHeader.width;
        int height = abs(infoHeader.height);
        int rowSize = ((width * 3 + 3) / 4) * 4;
        int dataSize = rowSize * height;

        data = malloc(dataSize);
        output = malloc(dataSize);

        fseek(file, header.offset, SEEK_SET);
        fread(data, 1, dataSize, file);
        fclose(file);

        // Aplicar el filtro Sobel con OpenMP
        sobel_filter_omp(data, output, width, height);

        // Guardar la imagen resultante
        sprintf(output_filename, "images/sobel_openmp_%d.bmp", img);
        FILE *outFile = fopen(output_filename, "wb");
        fwrite(&header, sizeof(BMPHeader), 1, outFile);
        fwrite(&infoHeader, sizeof(BMPInfoHeader), 1, outFile);
        fseek(outFile, header.offset, SEEK_SET);
        fwrite(output, 1, dataSize, outFile);
        fclose(outFile);

        // Medir el uso de memoria (aproximado)
        size_t memory_used = dataSize * 2 + sizeof(BMPHeader) + sizeof(BMPInfoHeader);
        printf("Imagen %d procesada con OpenMP. Memoria utilizada: %zu bytes\n", img, memory_used);

        free(data);
        free(output);
    }

    printf("Presione Enter para finalizar...");
    getchar();
    return 0;
}
