#ifndef SUPERVISOR_H
#define SUPERVISOR_H

#include <pthread.h>
#include <stdint.h>
#include "counter.h"

typedef struct {
    Counter* counters;      // arreglo de todos los counters
    int32_t  num_counters;  // cantidad total de counters
    volatile int* activa;   // flag global de simulacion
} Supervisor;

void init_supervisor(Supervisor* s, Counter* counters, int32_t num_counters,
                     volatile int* activa);
void destruir_supervisor(Supervisor* s);
void* hilo_supervisor(void* arg);

#endif // SUPERVISOR_H