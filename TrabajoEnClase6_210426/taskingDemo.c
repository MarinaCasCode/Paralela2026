// Write a program that prints either "A race car" or "A car race" and maximize parallelism
// Jimena Bejarano Sanchez C31074
// Marina Castro Peralta C31886
// gcc taskingDemo.c -o taskingDemo -fopenmp

// para correr en kabre:
// salloc --nodes=1 --ntasks=1 --cpus-per-task=4 --time=00:30:00 --partition=batch
// gcc taskingDemo.c -o taskingDemo -fopenmp
// ./taskingDemo


#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <omp.h>

int main() {
    srand(time(NULL));
    int order = rand() % 2;  // 0: race car, 1: car race

    #pragma omp parallel
    {
        #pragma omp single
        {
            #pragma omp task
            printf("A ");
            #pragma omp taskwait

            if (order == 0) {
                #pragma omp task
                printf("race ");
                #pragma omp taskwait

                #pragma omp task
                printf("car\n");
            } else {
                #pragma omp task
                printf("car ");
                #pragma omp taskwait

                #pragma omp task
                printf("race\n");
            }
        }
    }
    return 0;
}
