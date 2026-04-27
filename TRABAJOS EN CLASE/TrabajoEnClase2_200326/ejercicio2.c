// Hacer un programa en C en el que la tarea de los threads sea iterar 1000 veces sobre una variable compartida y hacerle ++
// compilacion: gcc -o ejercicio2 ejercicio2.c -lpthread
// Jimena Bejarano Sanchez C31074
// Marina Castro Peralta C31886

#include <stdio.h>
#include <pthread.h>
#define NUM_THREADS 20 // Número de threads a crear
#define ITERATIONS 1000 // Número de iteraciones que cada thread realizará

long sharedVariable = 0; // Variable compartida entre los threads

void* increment(void *threadid) { // Función que será ejecutada por cada thread
    long tid;
    tid = *(long*)threadid;
    for (int i = 0; i < ITERATIONS; i++) {
        sharedVariable++; // Incrementar la variable compartida
    }
}

int main() {
    pthread_t threads[NUM_THREADS];
    long tids[NUM_THREADS];
    long i;
    
    // Crear un nuevo thread que ejecutará la función increment
    for (i = 0; i < NUM_THREADS; i++) {
        tids[i] = i;
        pthread_create(&threads[i], NULL, increment, (void*)&tids[i]);
    }

    // esperar a que todos los threads terminen
    for (i = 0; i < NUM_THREADS; i++) {
        pthread_join(threads[i], NULL);
    }
    printf("Valor final de la variable compartida: %ld\n", sharedVariable); // Imprimir el valor final de la variable compartida
    pthread_exit(NULL); // Terminar el thread principal
    
    return 0;
}