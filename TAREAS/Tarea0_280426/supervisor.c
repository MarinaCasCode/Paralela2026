#include "supervisor.h"
#include <stdio.h> // para printf
#include <unistd.h> // para usleep
#include <time.h> // para clock_gettime 
#include <assert.h> // para assert que detiene el programa si se trata de hacer algo "imposible" como destruir un supervisor nulo
#include <stdlib.h> // para malloc y free

// initSupervisor 

void initSupervisor(Supervisor* s, Counter* contadores, int32_t numContadores, volatile int* activa) {
    assert(s != NULL); // Asegura que supervisor sea distinto a nulo 
    assert(contadores != NULL); // Asegura que el arreglo de counters sea distinto a nulo
    assert(activa != NULL); // Asegura que el flag de simulacion sea distinto

    s->contadores = contadores; // Asigna el arreglo de counters al supervisor
    s->numContadores = numContadores; // Asigna la cantidad de counters al supervisor
    s->activa = activa; // Asigna el puntero al flag global de simulacion al supervisor

    printf("Supervisor inicializado. Vigila a %d counters.\n", numContadores);

    }
    // destructor 
void destruirSupervisor(Supervisor* s) {
    assert (s != NULL); // Asegura que supervisor sea distinto a nulo
    // Como supervisor no tiene memoria propia solo guarda punteros al arreglo de counters 
    // por lo tanto no se libera memoria de counters ni de activa, solo se limpia el struct supervisor por si se quiere reutilizar
    s->contadores = NULL;
    s->numContadores = 0;
    s->activa = NULL;   
    printf("Supervisor destruido.\n");
    }

void* hiloSupervisor(void* arg) {
Supervisor* supervisor = (Supervisor*)arg; // convierte argumento a un puntero a Supervisor
assert(supervisor != NULL);

// Mientras la simulacion esté activa
// Supervisor chequea si hay algun counter en break y si ya pasó el tiempo del break para reabrirlo 
while (*(supervisor->activa)) {

    // Recorrido por todos los counters para revisar estado 
    for (int32_t i= 0; i < supervisor->numContadores; i++) {
        Counter* contador = &supervisor->contadores[i]; // puntero a counter actual
        pthread_mutex_lock(&contador->mutex); // Bloquea el mutex del counter para revisar su estado y tiempo de fin de break

        if (contador -> estado == COUNTER_ON_BREAK) {
            
        }
    }

}

}