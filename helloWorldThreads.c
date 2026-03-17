// Ejercicio 2: Hacer un simple Hello World con threads en C
// Jimena Bejarano Sanchez C31074
// Marina Castro Peralta C31886

#include <stdio.h>
#include <pthread.h>
#define NUM_THREADS 5 // Número de threads a crear

void* printHello(void* arg) { // Función que será ejecutada por cada thread
    int tid = *((int*)arg);
    printf("Hello, World!\n", tid);
    return NULL;
}

int main() {
    pthread_t threads[NUM_THREADS];
    int tids[NUM_THREADS];
    
    // Crear un nuevo thread que ejecutará la función printHello
    for (int i = 0; i < NUM_THREADS; i++) {
        tids[i] = i;
        pthread_create(&threads[i], NULL, printHello, &tids[i]);
    }
    
    return 0;
}