#ifndef BALANCER_H
#define BALANCER_H
#include "counter.h"
#include "queue.h"
#include <pthread.h>
#include <stdint.h>

typedef struct {
    Cola* colaEconomy;
    Cola* colaBusiness; 
    Cola* colaInternacional; // sobra 

    int32_t maxEnCola; // si una cola sobrepasa este valor, se pasa a internacional 
    int32_t tMaxEspera; // tiempo maximo de espera de un pasajero en businnes antes que suceda un priority bump
    volatile int* activa; // flag global de simulacion
} Balancer;

// incializar balancer con colas y tiempos 
void initBalancer(Balancer* b, Cola* colaEconomy, Cola* colaBusiness, Cola* colaInternacional, int32_t maxEnCola, int32_t tMaxEspera, volatile int* activa);

// liberar recursos 
void destruirBalancer(Balancer* b);

// funcion main del hilo balancer 
void* hiloBalancer(void* arg);

// para insertar pasajero al frente de la cola (prority bump)
void priorityBump(Cola* cola, void* data);

#endif // BALANCER_H