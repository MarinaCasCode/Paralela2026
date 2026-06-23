// cenatMD.cpp

// Compilacion: ver Makefile (mpicxx -O2 -fopenmp)
// Ejecucion:   mpiexec -np <#Ranks> ./cenatMD <N> <ITERACIONES> <BANDERA_IMP> <BANDERA_INI>
//   N            - cantidad de particulas POR proceso
//   ITERACIONES  - cantidad de iteraciones de la simulacion
//   BANDERA_IMP  - 1 = escribir archivo de posiciones cada 100 iteraciones
//   BANDERA_INI  - 1 = inicializar particulas en posiciones fijas (para validacion)

#include <cstdio>
#include <mpi.h>
#include <omp.h>

int main(int argc, char* argv[]) {
    MPI_Init(&argc, &argv);

    int rank = 0;   // identificador de este proceso dentro del comunicador
    int size = 0;   // cantidad total de procesos MPI
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    // Cada proceso reporta su rank y cuantos hilos OpenMP tiene disponibles.
    // Esto confirma que el binario es realmente hibrido (MPI + OpenMP).
    int max_threads = omp_get_max_threads();
    printf("[rank %d/%d] hilos OpenMP disponibles = %d\n", rank, size, max_threads);
    fflush(stdout);

    // Solo el rank 0 imprime un encabezado con los argumentos recibidos.
    if (rank == 0) {
        printf("=== cenatMD (Fase 0: verificacion MPI+OpenMP) ===\n");
        printf("Procesos MPI = %d\n", size);
        printf("Argumentos recibidos (%d): ", argc - 1);
        for (int i = 1; i < argc; i++) {
            printf("%s ", argv[i]);
        }
        printf("\n");
    }

    MPI_Finalize();
    return 0;
}