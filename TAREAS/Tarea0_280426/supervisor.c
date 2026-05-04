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
            // si contador esta en break, ver si ya paso el fin del break
            struct timespec ahora;
            clock_gettime(CLOCK_MONOTONIC, &ahora); // obtiene el tiempo actual para comparar con el tiempo de fin de break del counter
            
            // break terminado si el tiempo actual es mayor o igual al tiempo de fin de break del counter 
            if ((ahora.tv_sec > contador->tiempoFinBreak.tv_sec) ||
            (ahora.tv_sec == contador->tiempoFinBreak.tv_sec && 
            ahora.tv_nsec >= contador->tiempoFinBreak.tv_nsec)) {
                // el break ya terminó, se puede reabrir el counter
                printf("Supervisor: Counter %d en break terminó su descanso. Reabriendo...\n", contador->id);
                pthread_cond_signal(&contador->condReopen); // Señala al counter para que se despierte y reabra
                contador->estado = COUNTER_OPEN; 
            }
        }
        pthread_mutex_unlock(&contador->mutex); // Desbloquea el mutex del counter después de revisar su estado y tiempo de fin de break
    }
    // Pausa o duerme poco antes de volver a revisar para no estar consumiendo cpu
    // cada 50ms, suficiente para que no haya busy waiting
    struct timespec espera = {0, 50*1000000}; // 50 ms en nanosegundos
    nanosleep(&espera, NULL); // Duerme por el tiempo especificado antes
}
printf("Hilo supervisor termiando, simulación inactiva.\n");
return NULL; // Retorna NULL al finalizar el hilo

} // se cierra hilo de supervisor