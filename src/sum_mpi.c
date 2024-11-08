#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>

#define max_rows 100000
#define send_data_tag 2001
#define return_data_tag 2002

int array[max_rows];
int array2[max_rows];

int main(int argc, char **argv)
{
    long int sum, partial_sum;
    MPI_Status status;
    int my_id, root_process, ierr, i, num_rows, num_procs,
        an_id, num_rows_to_receive, avg_rows_per_process,
        sender, num_rows_received, start_row, end_row, num_rows_to_send;

    setbuf(stdout, NULL); // Deshabilitar el buffering de stdout

    // Inicializar MPI
    ierr = MPI_Init(&argc, &argv);

    root_process = 0;

    // Obtener el ID del proceso y el número total de procesos
    ierr = MPI_Comm_rank(MPI_COMM_WORLD, &my_id);
    ierr = MPI_Comm_size(MPI_COMM_WORLD, &num_procs);

    // Sincronizar los procesos antes de interactuar con el usuario
    MPI_Barrier(MPI_COMM_WORLD);

    if (my_id == root_process) {
        // Proceso maestro

        printf("Por favor, ingrese el número de elementos a sumar: ");
        fflush(stdout); // Forzar el vaciado del buffer
        scanf("%i", &num_rows);

        if (num_rows > max_rows) {
            printf("Demasiados números.\n");
            exit(1);
        }

        avg_rows_per_process = num_rows / num_procs;

        // Inicializar el arreglo
        for (i = 0; i < num_rows; i++) {
            array[i] = i + 1;
        }

        // Distribuir una porción del arreglo a cada proceso esclavo
        for (an_id = 1; an_id < num_procs; an_id++) {
            start_row = an_id * avg_rows_per_process;
            end_row = (an_id + 1) * avg_rows_per_process;

            if (an_id == num_procs - 1) {
                // Último proceso se encarga del resto
                end_row = num_rows;
            }

            num_rows_to_send = end_row - start_row;

            ierr = MPI_Send(&num_rows_to_send, 1, MPI_INT,
                            an_id, send_data_tag, MPI_COMM_WORLD);

            ierr = MPI_Send(&array[start_row], num_rows_to_send, MPI_INT,
                            an_id, send_data_tag, MPI_COMM_WORLD);
        }

        // Calcular la suma de la porción asignada al proceso maestro
        sum = 0;
        for (i = 0; i < avg_rows_per_process; i++) {
            sum += array[i];
        }

        printf("Suma %ld calculada por el proceso maestro\n", sum);

        // Recibir las sumas parciales de los procesos esclavos y calcular el total general
        for (an_id = 1; an_id < num_procs; an_id++) {
            ierr = MPI_Recv(&partial_sum, 1, MPI_LONG, MPI_ANY_SOURCE,
                            return_data_tag, MPI_COMM_WORLD, &status);

            sender = status.MPI_SOURCE;

            printf("Suma parcial %ld recibida del proceso %d\n", partial_sum, sender);

            sum += partial_sum;
        }

        printf("El total general es: %ld\n", sum);
    } else {
        // Procesos esclavos

        // Recibir el número de filas a procesar
        ierr = MPI_Recv(&num_rows_to_receive, 1, MPI_INT,
                        root_process, send_data_tag, MPI_COMM_WORLD, &status);

        // Recibir la porción del arreglo
        ierr = MPI_Recv(array2, num_rows_to_receive, MPI_INT,
                        root_process, send_data_tag, MPI_COMM_WORLD, &status);

        num_rows_received = num_rows_to_receive;

        // Calcular la suma parcial
        partial_sum = 0;
        for (i = 0; i < num_rows_received; i++) {
            partial_sum += array2[i];
        }

        // Enviar la suma parcial al proceso maestro
        ierr = MPI_Send(&partial_sum, 1, MPI_LONG, root_process,
                        return_data_tag, MPI_COMM_WORLD);
    }

    // Finalizar MPI
    ierr = MPI_Finalize();

    return 0;
}
