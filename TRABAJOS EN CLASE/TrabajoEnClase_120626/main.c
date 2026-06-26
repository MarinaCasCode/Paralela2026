// Patrón de intercambio de celdas fantasma (ghost cells / halo exchange):
//
//   Proceso r-1 | [ghost_izq | elem_0 … elem_{n-1} | ghost_der] | Proceso r+1
//                      ^                                    ^
//               recibe de r-1                        recibe de r+1
//               envía elem_0 a r-1               envía elem_{n-1} a r+1
// -------------------------------------------------------------------------

#include <mpi.h> // interfaz de paso de tasks, permite que diferentes nucleos de CPU se comuniquen entre si
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// se inicializa la parte local del arreglo con un patron simple, los extremos se mantienen en 0
static void inicializarArreglo(double *arr, int local_n, int offset, int N)
{
    for (int i = 0; i < local_n; i++) {
        int global_i = offset + i; // offset es el indice global del primer elemento local
        // Frontera global → 0; interior → valor = índice + 1
        arr[i + 1] = (global_i == 0 || global_i == N - 1) ? 0.0 : (double)(global_i + 1);
    }
}

// imprimir arreglo local incluyendo ghost cells
static void imprimirGlobal(double *arreglo, int N, int iter)
{
    printf("Después de la iteración %d: [", iter);
    for (int i = 0; i < N; i++)
        printf(" %.4f", arreglo[i]); // se imprimen los elementos del arreglo global
    printf(" ]\n");
}

// funcion main
int main(int argc, char *argv[]) {
    // inicializar MPI
    MPI_Init(&argc, &argv);

    int rango, num_procs;
    MPI_Comm_rank(MPI_COMM_WORLD, &rango);
    MPI_Comm_size(MPI_COMM_WORLD, &num_procs);

    // leer arg de la linea de comandos
    if (argc != 3) {
        if (rango == 0)
            fprintf(stderr, "Uso: %s <N> <M>\n", argv[0]); // standard error si no se pasan los argumentos correctos
        MPI_Finalize();
        return 1;
    }
    int N = atoi(argv[1]); // número total de elementos del arreglo 
    int M = atoi(argv[2]); // número de iteraciones del stencil
    // se usa atoi para convertir de string a entero, no se hace validacion de errores para simplificar
    
    if (N < num_procs) {
        if (rango == 0)
            fprintf(stderr, "Error: N (%d) debe ser >= número de procesos (%d)\n", N, num_procs);
        MPI_Finalize();
        return 1;
    }

    // mapping y aglomeración
    int base    = N / num_procs;
    int residuo = N % num_procs;

    int local_n = base + (rango < residuo ? 1 : 0);
    int offset  = rango * base + (rango < residuo ? rango : residuo);

    // layout local
    double *cur = (double *)calloc(local_n + 2, sizeof(double)); // estado actual
    double *nxt = (double *)calloc(local_n + 2, sizeof(double)); // estado siguiente
    if (!cur || !nxt) {
        fprintf(stderr, "Proceso %d: falla en la reserva de memoria\n", rango);
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    // Inicializar datos; las celdas fantasma comienzan en 0 (calloc)
    inicializarArreglo(cur, local_n, offset, N);
    memcpy(nxt, cur, (local_n + 2) * sizeof(double));

    // determinar rangos vecinos
    // mpi_proc_null es el proceso "nulo"
    int vecino_izq = (rango == 0) ? MPI_PROC_NULL : rango - 1;
    int vecino_der = (rango == num_procs - 1) ? MPI_PROC_NULL : rango + 1;

    // for de iteraciones
    for (int iter = 0; iter < M; iter++) {

        // COMUNICACIÓN: intercambio de halos (ghost cell exchange)
        //
        // Patrón de comunicación: vecinos más cercanos (1D, extremos abiertos).
        //
        // Cada proceso envía su elemento más a la izquierda al vecino izquierdo,
        // y su elemento más a la derecha al vecino derecho,
        // recibiendo a la vez en sus celdas fantasma.
        //
        // MPI_Sendrecv evita deadlocks sin necesidad de ordenar manualmente
        // los envíos y recepciones.
        MPI_Sendrecv(&cur[1], 1, MPI_DOUBLE, vecino_izq, 0,           // enviar vecino izquierdo
                     &cur[local_n + 1], 1, MPI_DOUBLE, vecino_der, 0, // recibir vecino derecho
                     MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        MPI_Sendrecv(&cur[local_n], 1, MPI_DOUBLE, vecino_der, 1,     // enviar vecino derecho
                     &cur[0], 1, MPI_DOUBLE, vecino_izq, 1,           // recibir vecino izquierdo
                     MPI_COMM_WORLD, MPI_STATUS_IGNORE);

        // CÓMPUTO: stencil 1D (vecinos más cercanos)
        for (int i = 1; i <= local_n; i++) {
            int global_i = offset + i - 1; // índice global del elemento cur[i]

            // frontera fija
            if (global_i == 0 || global_i == N - 1) {
                nxt[i] = cur[i]; // frontera fija
            } else {
                nxt[i] = (cur[i - 1] + cur[i] + cur[i + 1]) / 3.0; // stencil
            }
        }

        // INTERCAMBIO DE PUNTEROS: cur <-> nxt
        double *tmp = cur;
        cur = nxt;
        nxt = tmp;
    }

    // recoleccion de datos en proceso 0
    int    *conteos    = NULL;
    int    *desplazam  = NULL;
    double *global_arr = NULL;

    if (rango == 0) {
        conteos    = (int *)malloc(num_procs * sizeof(int));
        desplazam  = (int *)malloc(num_procs * sizeof(int));
        global_arr = (double *)malloc(N * sizeof(double));

        // Calcular cuántos elementos envía cada proceso y desde qué posición
        for (int r = 0; r < num_procs; r++) {
            conteos[r]   = base + (r < residuo ? 1 : 0);
            desplazam[r] = r * base + (r < residuo ? r : residuo);
        }
    }

    // cada proceso envia porcion local (sin ghost)
    MPI_Gatherv(
        &cur[1], local_n, MPI_DOUBLE,               // datos locales a enviar
        global_arr, conteos, desplazam, MPI_DOUBLE, // buffer de recepción en proceso 0
        0, MPI_COMM_WORLD);

    // Solo el proceso 0 imprime el resultado final
    if (rango == 0) {
        imprimirGlobal(global_arr, N, M);
        free(conteos);
        free(desplazam);
        free(global_arr);
    }

    // liberar memoria local y finalizar MPI
    free(cur);
    free(nxt);
    MPI_Finalize();
    return 0;
}