// Ejercicio 2: aproximacion de pi con loop constructs, paralelizado con OpenMP
// Jimena Bejarano Sanchez C31074
// Marina Castro Peralta C31886
// gcc aproximacionPi3.c -o aproximacionPi3 -fopenmp

#include <omp.h>
#include <stdio.h>

#define NUM_THREADS 4

static long num_steps = 100000;
double step;

int main() {
    double pi, sum = 0.0;

    step = 1.0 / (double) num_steps;
    omp_set_num_threads(NUM_THREADS);

    double tdata = omp_get_wtime();

    // parallel for: divide el loop entre threads; reduction acumula sum de forma segura
    #pragma omp parallel for reduction(+:sum)
    for (int i = 0; i < num_steps; i++) {
        double x = (i + 0.5) * step;
        sum += 4.0 / (1.0 + x * x);
    }

    tdata = omp_get_wtime() - tdata;

    pi = step * sum;
    printf("pi = %f in %f secs\n", pi, tdata);
    return 0;
}
