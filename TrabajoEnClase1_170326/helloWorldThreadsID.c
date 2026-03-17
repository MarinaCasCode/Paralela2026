// Ejercicio 3: Hacer un simple Hello World con threads en C que identifica nucleos
// compilacion: gcc -o helloWorldThreadsID helloWorldThreadsID.c -lpthread
// Jimena Bejarano Sanchez C31074
// Marina Castro Peralta C31886

#include <stdio.h>
#include <pthread.h>
#define NUM_THREADS 5 // Número de threads a crear

void* printHello(void *threadid) { // Función que será ejecutada por cada thread
    long tid;
    tid = *(long*)threadid;
    printf("Hello, World! Soy el hilo: %ld\n", tid);
    pthread_exit(NULL); // Terminar el thread actual
}

int main() {
    pthread_t threads[NUM_THREADS];
    int tids[NUM_THREADS];
    long i;
    
    // Crear un nuevo thread que ejecutará la función printHello
    for (i = 0; i < NUM_THREADS; i++) {
        printf("Creando hilo %ld\n", i);
        tids[i] = i;
        pthread_create(&threads[i], NULL, printHello, (void*)&tids[i]);
    }

    pthread_exit(NULL); // Terminar el thread principal
    
    return 0;
}