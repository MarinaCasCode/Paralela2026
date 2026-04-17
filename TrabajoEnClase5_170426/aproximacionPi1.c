// Ejercicio 1: aproximacion de pi con regla trapezoidal, paralelizado con OpenMP
// Jimena Bejarano Sanchez C31074
// Marina Castro Peralta C31886
// gcc aproximacionPi1.c -o aproximacionPi1 -fopenmp

#include <omp.h>
#include <stdio.h>

#define NUM_THREADS 4

static long num_steps = 100000;
double step;

int main() {
    double pi, sum = 0.0;
    // cada hilo guarda su suma parcial en su propia posicion: no hay race condition
    double partial_sum[NUM_THREADS] = {0.0};

    step = 1.0 / (double) num_steps;
    omp_set_num_threads(NUM_THREADS);

    double tdata = omp_get_wtime();

    #pragma omp parallel
    {
        int id = omp_get_thread_num();
        int nthreads = omp_get_num_threads();

        // cada hilo agarra los indices id, id+nthreads, id+2*nthreads, etc.
        for (int i = id; i < num_steps; i += nthreads) {
            double x = (i + 0.5) * step;
            partial_sum[id] += 4.0 / (1.0 + x * x); // f(x) = 4/(1+x^2)
        }
    }

    // sumamos los resultados de cada hilo de forma serial
    for (int i = 0; i < NUM_THREADS; i++)
        sum += partial_sum[i];

    tdata = omp_get_wtime() - tdata;

    pi = step * sum;
    printf("pi = %f in %f secs\n", pi, tdata);
    return 0;
}
