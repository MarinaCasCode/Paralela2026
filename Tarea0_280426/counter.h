#ifndef COUNTER_H
#define COUNTER_H

#include <pthread.h>
#include <stdint.h>
#include <time.h>
#include "queue.h"
#include "pasajero.h"

// Ciclo de vida: OPEN -> SERVING -> ON_BREAK -> OPEN
typedef enum {
    COUNTER_OPEN     = 0,
    COUNTER_SERVING  = 1,
    COUNTER_ON_BREAK = 2
} EstadoCounter;

typedef enum {
    TIPO_ECONOMY       = 0,
    TIPO_BUSINESS      = 1,
    TIPO_INTERNATIONAL = 2
} TipoCounter;

typedef struct {
    int32_t id;
    TipoCounter tipo;
    EstadoCounter estado;

    // Mutex propio del counter: solo se usa para cambios de estado y stats
    // Orden de adquisicion: siempre queue->mutex ANTES que counter->mutex
    pthread_mutex_t mutex;
    pthread_cond_t cond_reopen; // el Supervisor señala aqui al reabrir

    int k_min, k_max;
    int k_actual;        // K aleatorio del turno actual (en [k_min, k_max])
    int atendidos_turno;  // pasajeros atendidos desde el ultimo break
    unsigned int rand_seed; // semilla por hilo para rand_r

    Cola* cola;           // cola de la que este counter consume
    volatile int* activa; // puntero al flag global de simulacion

    struct timespec tiempo_fin_break; // cuando termina el break (para el Supervisor)

    // Estadisticas
    int64_t total_atendidos;
    long long suma_espera_ms;
    long long suma_servicio_ms;
} Counter;

void init_counter(Counter* c, int32_t id, TipoCounter tipo, Cola* cola,
                  int k_min, int k_max, volatile int* activa);
void destruir_counter(Counter* c);
void* hilo_counter(void* arg);

#endif // COUNTER_H
