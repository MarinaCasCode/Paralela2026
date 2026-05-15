// heat3d.cpp
// Tarea 1 - CI-0117 Programacion Paralela y Concurrente
// Simulacion de la ecuacion de calor 3D en estado estacionario.
// Metodo de Jacobi con estencil de 6 vecinos
//
// Compilacion: ver Makefile (g++ -O2 -fopenmp -Wall)

#include <cstdio>    // printf
#include <cstdlib>   // malloc, free, exit
#include <cstring>   // memcpy (se usara en el paso de actualizacion)
#include <omp.h>     // OpenMP: directivas y omp_get_wtime

// Parametros base que pide el enunciado
#define N         100    // dimension de la malla N x N x N
#define NUM_STEPS 1000   // numero de iteraciones de Jacobi

int main() {
    printf("Heat3D - configuracion inicial\n");
    printf("N         = %d\n", N);
    printf("NUM_STEPS = %d\n", NUM_STEPS);
    return 0;
}