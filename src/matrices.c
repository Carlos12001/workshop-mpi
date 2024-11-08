#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/resource.h>

#define MATRIX_SIZE 4  // Dimensión de las matrices

// Función para inicializar una matriz con valores secuenciales
void initialize_matrix(int matrix[MATRIX_SIZE][MATRIX_SIZE]) {
    int value = 1;
    for (int row = 0; row < MATRIX_SIZE; row++) {
        for (int col = 0; col < MATRIX_SIZE; col++) {
            matrix[row][col] = value++;
        }
    }
}

// Función para imprimir una matriz
void display_matrix(int matrix[MATRIX_SIZE][MATRIX_SIZE]) {
    for (int row = 0; row < MATRIX_SIZE; row++) {
        for (int col = 0; col < MATRIX_SIZE; col++) {
            printf("%4d ", matrix[row][col]);
        }
        printf("\n");
    }
}

int main(int argc, char *argv[]) {
    int world_rank, world_size;
    int A[MATRIX_SIZE][MATRIX_SIZE];
    int B[MATRIX_SIZE][MATRIX_SIZE];
    int result[MATRIX_SIZE][MATRIX_SIZE];

    // Inicializar MPI
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);

    // Variables para métricas
    struct rusage usage_stats;
    long bytes_sent = 0;
    long bytes_received = 0;
    double comp_start, comp_end, comp_time;
    double comm_start, comm_end, comm_time;

    // Inicializar matrices en el proceso maestro
    if (world_rank == 0) {
        initialize_matrix(A);
        initialize_matrix(B);
    }

    // Compartir la matriz B con todos los procesos
    MPI_Bcast(B, MATRIX_SIZE * MATRIX_SIZE, MPI_INT, 0, MPI_COMM_WORLD);

    // Determinar el número de filas para cada proceso
    int rows_per_proc = MATRIX_SIZE / world_size;
    int remaining_rows = MATRIX_SIZE % world_size;
    int my_rows = rows_per_proc + (world_rank < remaining_rows ? 1 : 0);

    // Calcular desplazamiento de filas
    int row_offset = world_rank * rows_per_proc + (world_rank < remaining_rows ? world_rank : remaining_rows);

    // Buffer para las filas asignadas a este proceso
    int local_A[my_rows][MATRIX_SIZE];
    int local_result[my_rows][MATRIX_SIZE];

    // Distribuir las filas de A a cada proceso
    int *sendcounts = NULL;
    int *displs = NULL;
    if (world_rank == 0) {
        sendcounts = malloc(world_size * sizeof(int));
        displs = malloc(world_size * sizeof(int));

        for (int i = 0; i < world_size; i++) {
            int rows = rows_per_proc + (i < remaining_rows ? 1 : 0);
            sendcounts[i] = rows * MATRIX_SIZE;
            displs[i] = (i * rows_per_proc + (i < remaining_rows ? i : remaining_rows)) * MATRIX_SIZE;
        }
    }

    MPI_Scatterv(&A[0][0], sendcounts, displs, MPI_INT,
                 &local_A[0][0], my_rows * MATRIX_SIZE, MPI_INT,
                 0, MPI_COMM_WORLD);

    if (world_rank == 0) {
        free(sendcounts);
        free(displs);
    }

    // Sincronizar antes de iniciar el cómputo
    MPI_Barrier(MPI_COMM_WORLD);

    // Iniciar medición de tiempo de cómputo
    comp_start = MPI_Wtime();

    // Multiplicación de matrices parcial
    for (int i = 0; i < my_rows; i++) {
        for (int j = 0; j < MATRIX_SIZE; j++) {
            local_result[i][j] = 0;
            for (int k = 0; k < MATRIX_SIZE; k++) {
                local_result[i][j] += local_A[i][k] * B[k][j];
            }
        }
    }

    // Finalizar medición de tiempo de cómputo
    comp_end = MPI_Wtime();
    comp_time = comp_end - comp_start;

    // Iniciar medición de tiempo de comunicación
    comm_start = MPI_Wtime();

    // Recolectar los resultados parciales en el proceso maestro
    int *recvcounts = NULL;
    int *recvdispls = NULL;
    if (world_rank == 0) {
        recvcounts = malloc(world_size * sizeof(int));
        recvdispls = malloc(world_size * sizeof(int));

        for (int i = 0; i < world_size; i++) {
            int rows = rows_per_proc + (i < remaining_rows ? 1 : 0);
            recvcounts[i] = rows * MATRIX_SIZE;
            recvdispls[i] = (i * rows_per_proc + (i < remaining_rows ? i : remaining_rows)) * MATRIX_SIZE;
        }
    }

    MPI_Gatherv(&local_result[0][0], my_rows * MATRIX_SIZE, MPI_INT,
                &result[0][0], recvcounts, recvdispls, MPI_INT,
                0, MPI_COMM_WORLD);

    // Actualizar métricas de comunicación
    bytes_sent += my_rows * MATRIX_SIZE * sizeof(int);
    bytes_received += my_rows * MATRIX_SIZE * sizeof(int);

    // Finalizar medición de tiempo de comunicación
    comm_end = MPI_Wtime();
    comm_time = comm_end - comm_start;

    // Obtener uso de recursos
    getrusage(RUSAGE_SELF, &usage_stats);

    // Mostrar métricas de cada proceso con mensajes diferentes
    printf(">>> Proceso [%d] Reporte de Métricas <<<\n", world_rank);
    printf("Memoria Máxima Usada: %ld KB\n", usage_stats.ru_maxrss);
    printf("Tiempo de Cómputo: %.6f segundos\n", comp_time);
    printf("Tiempo de Comunicación: %.6f segundos\n", comm_time);
    printf("Datos Enviados: %ld bytes\n", bytes_sent);
    printf("Datos Recibidos: %ld bytes\n", bytes_received);
    printf("-------------------------------\n\n");

    // El proceso maestro muestra el resultado final
    if (world_rank == 0) {
        printf("===== Resultado de la Multiplicación de Matrices =====\n");
        display_matrix(result);

        // Esperar a que el usuario presione Enter antes de finalizar
        printf("\nPresione Enter para finalizar...");
        getchar();
    }

    // Finalizar MPI
    MPI_Finalize();
    return 0;
}
