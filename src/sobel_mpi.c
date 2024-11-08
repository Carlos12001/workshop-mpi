// sobel_mpi.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mpi.h>
#include <math.h>
#include <sys/resource.h>

// Estructuras para manejar el encabezado BMP
#pragma pack(push, 1)
typedef struct {
    unsigned short type;       // Tipo de archivo (debe ser 'BM' para un archivo BMP válido)
    unsigned int size;         // Tamaño del archivo en bytes
    unsigned short reserved1;
    unsigned short reserved2;
    unsigned int offset;       // Desplazamiento a los datos de píxeles
} BMPHeader;

typedef struct {
    unsigned int size;           // Tamaño de esta estructura en bytes
    int width;                   // Ancho de la imagen en píxeles
    int height;                  // Altura de la imagen en píxeles
    unsigned short planes;       // Número de planos de color (debe ser 1)
    unsigned short bitCount;     // Número de bits por píxel
    unsigned int compression;    // Método de compresión utilizado
    unsigned int imageSize;      // Tamaño de los datos de imagen en bytes
    int xPixelsPerMeter;         // Resolución horizontal en píxeles por metro
    int yPixelsPerMeter;         // Resolución vertical en píxeles por metro
    unsigned int colorsUsed;     // Número de colores en la paleta
    unsigned int colorsImportant;// Número de colores importantes
} BMPInfoHeader;
#pragma pack(pop)

// Función para aplicar el filtro Sobel en una porción de la imagen
void sobel_filter(unsigned char *data, BMPInfoHeader infoHeader, unsigned char *newdata, int start, int end) {
    int width = infoHeader.width;
    int rowSize = ((width * 3 + 3) & (~3)); // Alineación a 4 bytes
    int height = abs(infoHeader.height);

    int Gx[3][3] = {
        {-1, 0, 1},
        {-2, 0, 2},
        {-1, 0, 1}
    };
    int Gy[3][3] = {
        {-1, -2, -1},
        { 0,  0,  0},
        { 1,  2,  1}
    };

    unsigned char *grayData = (unsigned char *)malloc(width * height);
    if (grayData == NULL) {
        fprintf(stderr, "Error al asignar memoria para grayData.\n");
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    // Convertir a escala de grises
    for (int y = start; y < end; y++) {
        for (int x = 0; x < width; x++) {
            int pos_rgb = y * rowSize + x * 3;
            int pos_gray = y * width + x;
            unsigned char blue = data[pos_rgb];
            unsigned char green = data[pos_rgb + 1];
            unsigned char red = data[pos_rgb + 2];
            unsigned char gray = (unsigned char)(0.3 * red + 0.59 * green + 0.11 * blue);
            grayData[pos_gray] = gray;
        }
    }

    // Aplicar el filtro Sobel
    for (int y = start + 1; y < end - 1; y++) {
        for (int x = 1; x < width - 1; x++) {
            int gx = 0;
            int gy = 0;

            for (int i = -1; i <=1; i++) {
                for (int j = -1; j <=1; j++) {
                    int pixel = grayData[(y + i) * width + (x + j)];
                    gx += Gx[i + 1][j + 1] * pixel;
                    gy += Gy[i + 1][j + 1] * pixel;
                }
            }

            int magnitude = (int)sqrt(gx * gx + gy * gy);
            if (magnitude > 255) magnitude = 255;
            unsigned char edgeVal = (unsigned char)magnitude;

            int pos_rgb = y * rowSize + x * 3;
            newdata[pos_rgb] = edgeVal;
            newdata[pos_rgb + 1] = edgeVal;
            newdata[pos_rgb + 2] = edgeVal;
        }
    }
    free(grayData);
}

int main(int argc, char *argv[]) {
    MPI_Init(&argc, &argv);

    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    for (int img = 6; img <= 10; img++) {
        BMPHeader bmpHeader;
        BMPInfoHeader bmpInfoHeader;
        unsigned char *data = NULL;

        int height, width, rowSize, totalSize;
        int rows_per_process, remaining_rows;
        int localHeight, localSize;
        int *sendcounts = NULL;
        int *displs = NULL;

        char input_filename[50];
        sprintf(input_filename, "images/%d.bmp", img);

        if (rank == 0) {
            FILE *inputFile = fopen(input_filename, "rb");
            if (inputFile == NULL) {
                perror("Error abriendo el archivo de entrada");
                MPI_Abort(MPI_COMM_WORLD, 1);
            }

            fread(&bmpHeader, sizeof(BMPHeader), 1, inputFile);
            if (bmpHeader.type != 0x4D42) {
                printf("El archivo no es un BMP válido\n");
                fclose(inputFile);
                MPI_Abort(MPI_COMM_WORLD, 1);
            }

            fread(&bmpInfoHeader, sizeof(BMPInfoHeader), 1, inputFile);

            width = bmpInfoHeader.width;
            height = abs(bmpInfoHeader.height);
            rowSize = ((width * 3 + 3) & (~3)); // Alineación a 4 bytes
            totalSize = rowSize * height;

            data = (unsigned char *)malloc(totalSize);
            if (data == NULL) {
                fprintf(stderr, "No se pudo asignar memoria para los datos de la imagen.\n");
                fclose(inputFile);
                MPI_Abort(MPI_COMM_WORLD, 1);
            }

            fseek(inputFile, bmpHeader.offset, SEEK_SET);
            fread(data, 1, totalSize, inputFile);
            fclose(inputFile);

            // Preparar los parámetros para la distribución de datos
            rows_per_process = height / size;
            remaining_rows = height % size;

            sendcounts = (int *)malloc(size * sizeof(int));
            displs = (int *)malloc(size * sizeof(int));

            int offset = 0;
            for (int i = 0; i < size; i++) {
                int lh = rows_per_process;
                if (i < remaining_rows) {
                    lh += 1;
                }
                sendcounts[i] = lh * rowSize;
                displs[i] = offset;
                offset += sendcounts[i];
            }
        }

        // Difundir los encabezados y parámetros
        MPI_Bcast(&bmpHeader, sizeof(BMPHeader), MPI_BYTE, 0, MPI_COMM_WORLD);
        MPI_Bcast(&bmpInfoHeader, sizeof(BMPInfoHeader), MPI_BYTE, 0, MPI_COMM_WORLD);

        if (rank != 0) {
            width = bmpInfoHeader.width;
            height = abs(bmpInfoHeader.height);
            rowSize = ((width * 3 + 3) & (~3));
            totalSize = rowSize * height;

            rows_per_process = height / size;
            remaining_rows = height % size;

            sendcounts = (int *)malloc(size * sizeof(int));
            displs = (int *)malloc(size * sizeof(int));
        }

        // Difundir 'sendcounts' y 'displs' a todos los procesos
        MPI_Bcast(sendcounts, size, MPI_INT, 0, MPI_COMM_WORLD);
        MPI_Bcast(displs, size, MPI_INT, 0, MPI_COMM_WORLD);

        localSize = sendcounts[rank];
        localHeight = localSize / rowSize;

        unsigned char *subData = (unsigned char *)malloc(localSize);
        unsigned char *subDataProcessed = (unsigned char *)malloc(localSize);

        // Distribuir los datos a los procesos
        MPI_Scatterv(data, sendcounts, displs, MPI_UNSIGNED_CHAR,
                     subData, localSize, MPI_UNSIGNED_CHAR,
                     0, MPI_COMM_WORLD);

        // Variables para métricas
        struct rusage usage_stats;
        long bytes_sent = 0;
        long bytes_received = 0;
        double comp_start, comp_end, comp_time;
        double comm_start, comm_end, comm_time;

        // Iniciar medición de tiempo de cómputo
        comp_start = MPI_Wtime();

        int start = 0;
        int end = localHeight;

        // Aplicar el filtro Sobel en cada proceso
        sobel_filter(subData, bmpInfoHeader, subDataProcessed, start, end);

        // Finalizar medición de tiempo de cómputo
        comp_end = MPI_Wtime();
        comp_time = comp_end - comp_start;

        // Iniciar medición de tiempo de comunicación
        comm_start = MPI_Wtime();

        // Recopilar los datos procesados en el proceso 0
        unsigned char *newData = NULL;
        if (rank == 0) {
            newData = (unsigned char *)malloc(totalSize);
        }

        MPI_Gatherv(subDataProcessed, localSize, MPI_UNSIGNED_CHAR,
                    newData, sendcounts, displs, MPI_UNSIGNED_CHAR,
                    0, MPI_COMM_WORLD);

        // Actualizar métricas de comunicación
        bytes_sent += localSize;
        bytes_received += localSize;

        // Finalizar medición de tiempo de comunicación
        comm_end = MPI_Wtime();
        comm_time = comm_end - comm_start;

        // Obtener uso de recursos
        getrusage(RUSAGE_SELF, &usage_stats);

        // Mostrar métricas de cada proceso
        printf(">>> Proceso [%d] Reporte de Métricas para imagen %d <<<\n", rank, img);
        printf("Memoria Máxima Usada: %ld KB\n", usage_stats.ru_maxrss);
        printf("Tiempo de Cómputo: %.6f segundos\n", comp_time);
        printf("Tiempo de Comunicación: %.6f segundos\n", comm_time);
        printf("Datos Enviados: %ld bytes\n", bytes_sent);
        printf("Datos Recibidos: %ld bytes\n", bytes_received);
        printf("-------------------------------\n\n");

        if (rank == 0) {
            // Guardar la imagen procesada
            char output_filename[50];
            sprintf(output_filename, "images/sobel_mpi_%d.bmp", img);
            FILE *outputFile = fopen(output_filename, "wb");
            if (!outputFile) {
                printf("No se pudo crear el archivo de salida\n");
                MPI_Abort(MPI_COMM_WORLD, 1);
            }

            fwrite(&bmpHeader, sizeof(BMPHeader), 1, outputFile);
            fwrite(&bmpInfoHeader, sizeof(BMPInfoHeader), 1, outputFile);
            fseek(outputFile, bmpHeader.offset, SEEK_SET);
            fwrite(newData, 1, totalSize, outputFile);

            fclose(outputFile);
            free(newData);
            free(data);
            free(sendcounts);
            free(displs);
        }

        free(subData);
        free(subDataProcessed);
    }

    if (rank == 0) {
        printf("Presione Enter para finalizar...");
        getchar();
    }

    MPI_Finalize();
    return 0;
}
