// heat3d.cpp
// Tarea 1 - CI-0117 Programacion Paralela y Concurrente
// Simulacion de la ecuacion de calor 3D en estado estacionario.
// Metodo de Jacobi con estencil de 6 vecinos
//
// Compilacion: ver Makefile (g++ -O2 -fopenmp -Wall)

#include <cstdio>    // printf
#include <cstdlib>   // malloc, free, exit
#include <cstring>   // memcpy
#include <omp.h>     // OpenMP: directivas y omp_get_wtime

// Parametros base que pide el enunciado
#define N         100    // dimension de la malla N x N x N
#define NUM_STEPS 1000   // numero de iteraciones de Jacobi

#define TEMP_COLD  0.0     // cara inferior
#define TEMP_HOT   100.0   // las otras cinco caras

// Conversion de coordenadas 3D (i,j,k) a indice lineal
// La malla se guarda como un bloque plano contiguo de N*N*N doubles
// Orden: i es el indice mas externo, k el mas interno
static inline int idx(int i, int j, int k) {
    return (i * N * N) + (j * N) + k;
}

// Asigna un arreglo 3D (bloque plano de N*N*N doubles) con malloc
// Termina el programa si malloc falla
static double* allocate_grid() {
    double* grid = (double*) malloc((size_t) N * N * N * sizeof(double));
    if (grid == NULL) {
        fprintf(stderr, "Error: no se pudo asignar memoria para la malla\n");
        exit(EXIT_FAILURE);
    }
    return grid;
}

// Libera un arreglo asignado con allocate_grid
static void free_grid(double* grid) {
    free(grid);
}

// Inicializa la malla: condiciones de frontera e interior.
static void initialize(double* array_old, double* array_new) {
    const double interior_value =
        (5.0 * TEMP_HOT + 1.0 * TEMP_COLD) / 6.0;

    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
            for (int k = 0; k < N; k++) {
                bool is_boundary =
                    (i == 0) || (i == N - 1) ||
                    (j == 0) || (j == N - 1) ||
                    (k == 0) || (k == N - 1);

                double value;
                if (is_boundary) {
                    if (i == 0) {
                        value = TEMP_COLD;   // cara inferior
                    } else {
                        value = TEMP_HOT;    // las otras cinco caras
                    }
                } else {
                    value = interior_value;  // punto interior
                }

                array_old[idx(i, j, k)] = value;
            }
        }
    }

    // array_new arranca identico a array_old (fronteras incluidas)
    memcpy(array_new, array_old, (size_t) N * N * N * sizeof(double));
}

int main() {
    printf("Heat3D - configuracion inicial\n");
    printf("N         = %d\n", N);
    printf("NUM_STEPS = %d\n", NUM_STEPS);

    // Asignacion de los dos arreglos de Jacobi
    double* array_old = allocate_grid();
    double* array_new = allocate_grid();

    printf("Memoria asignada correctamente para array_old y array_new\n");

    // Inicializacion: condiciones de frontera e interior
    initialize(array_old, array_new);

    printf("Malla inicializada (fronteras e interior)\n");

    // Liberacion de memoria
    free_grid(array_old);
    free_grid(array_new);

    printf("Memoria liberada correctamente\n");
    return 0;
}