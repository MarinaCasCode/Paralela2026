// Ejercicio 2: aproximacion de pi con synchronization constructs, paralelizado con OpenMP
// Jimena Bejarano Sanchez C31074
// Marina Castro Peralta C31886
// gcc aproximacionPi2.c -o aproximacionPi2 -fopenmp

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

    // cada hilo calcula su porcion de la suma
    #pragma omp parallel
    {
        int id = omp_get_thread_num();
        int nthreads = omp_get_num_threads();
        double local_sum = 0.0; // acumulador local para evitar race condition

        for (int i = id; i < num_steps; i += nthreads) {
            double x = (i + 0.5) * step;
            local_sum += 4.0 / (1.0 + x * x);
        }

        // critical: solo un hilo a la vez actualiza la suma global
        #pragma omp critical
        {
            sum += local_sum;
        }
    }

    tdata = omp_get_wtime() - tdata;

    pi = step * sum;
    printf("pi = %f in %f secs\n", pi, tdata);
    return 0;
}
