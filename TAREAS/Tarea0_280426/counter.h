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
    COUNTER_ON_BREAK = 2 // se usa para que el Supervisor sepa que el counter no está atendiendo pero tampoco está cerrado, y así no se cierra por inactividad
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
    pthread_cond_t condReopen; // el Supervisor señala aqui al reabrir

    int kMin, kMax;
    int kActual;        // K aleatorio del turno actual (en [kMin, kMax])
    int atendidosTurno;  // pasajeros atendidos desde el ultimo break
    unsigned int randSeed; // semilla por hilo para rand_r

    Cola* cola;           // cola de la que este counter consume
    volatile int* activa; // puntero al flag global de simulacion

    struct timespec tiempoFinBreak; // cuando termina el break (para el Supervisor)

    // Estadisticas
    int64_t totalAtendidos;
    long long sumaEsperaMs;
    long long sumaServicioMs;
} Counter;

void init_counter(Counter* c, int32_t id, TipoCounter tipo, Cola* cola,
                  int kMin, int kMax, volatile int* activa);
void destruirCounter(Counter* c);
void* hilo_counter(void* arg);

#endif // COUNTER_H
