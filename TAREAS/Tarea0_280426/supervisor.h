#ifndef SUPERVISOR_H
#define SUPERVISOR_H

#include <pthread.h>
#include <stdint.h>
#include "counter.h"

typedef struct {
    Counter* contadores;      // arreglo de todos los counters
    int32_t  numContadores;  // cantidad total de counters
    volatile int* activa;   // flag global de simulacion
} Supervisor;

void initSupervisor(Supervisor* s, Counter* contadores, int32_t numContadores, volatile int* activa);

void destruirSupervisor(Supervisor* s);

// Recorre counters buscando los que esten en break, compara el tiempo actual con tiempofinbreak y si ya paso manda cond signal para despewrtar al counter, se duerme 50ms antes de volber a revisar para evitar busy waiting
void* hiloSupervisor(void* arg);

#endif // SUPERVISOR_H