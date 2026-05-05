#include "counter.h"
#include "pasajero.h"

#include <stdlib.h> // para malloc y free
#include <assert.h> // para assert que detiene el programa si se trata de hacer algo "imposible" como destruir un counter nulo
#include <stdio.h> // para printf
#include <unistd.h> // para sleep
#include <time.h> // para clock_gettime y nanosleep


// funciones internas

// elector de k aleatorio en el turno actual dle counter
static int elegirK(Counter* contador) {
    int rango = contador ->kMax - contador->kMin + 1; // Calcula el rango de valores posibles para K
    return contador->kMin + rand_r(&contador->randSeed) % rango; //
}

// Nombre para el tipo de counter 
// para que se vea en el print 
static const char* nombreCounter (TipoCounter tipo) {
    switch (tipo) { // Devuelve el nombre del tipo de counter según su valor
        case TIPO_ECONOMY:
            return "Economy";
        case TIPO_BUSINESS:
            return "Business";
        case TIPO_INTERNATIONAL:
            return "International";
        default:
            return "Desconocido"; // Si el tipo no es reconocido
    }
}

// init  : para inicializar un counter, se le asigna su ID, tipo, cola de la que va a consumir, valores de K, y el flag global de simulacion. Se inicializan las estadisticas y el mutex del counter
void initCounter(Counter* contador, int32_t id, TipoCounter tipo, Cola* cola, int kmin, int kmax, volatile int* activa) { // se usa volatile porque el valor de activa puede cambiar en cualquier momento por otro hilo, y así se le indica al compilador que no optimice el acceso a esa variable asumiendo que no cambia
    assert(contador != NULL); // Asegura que el counter no sea nulo
    assert(cola != NULL); // Asegura que la cola no sea nula
    assert(activa != NULL); // Asegura que el flag de simulacion no sea nulo

    contador ->id = id; // Asigna el ID al counter
    contador->tipo = tipo; // Asigna el tipo al counter
    contador->estado = COUNTER_OPEN; // Inicializa el estado del counter a OPEN
    contador->cola = cola; // Asigna la cola al counter
    contador->kMin = kmin; // Asigna el valor mínimo de K al counter
    contador->kMax = kmax; // Asigna el valor máximo de K al counter
    contador->activa = activa; // Asigna el puntero al flag global de simulacion al counter
    contador->atendidosTurno = 0; // Inicializa el contador de atendidos
    contador->totalAtendidos = 0; // Inicializa el total de atendidos a 0
    contador->sumaEsperaMs = 0; // Inicializa la suma de tiempos de espera a 0
    contador->sumaServicioMs = 0; // Inicializa la suma de tiempos de

    // valor inicial (semilla) única por hilo
    contador->randSeed = (unsigned int)(time(NULL)) ^ (unsigned int)(id * 2654435761u);

    // Se elige primer k del turno 
    contador->kActual = elegirK(contador); 

    pthread_mutex_init(&contador->mutex, NULL);
    pthread_cond_init(&contador->condReopen, NULL);

    printf("Counter %d (%s) inicializado con K en [%d, %d]\n", id, nombreCounter(tipo), kmin, kmax);
}

void destruirCounter(Counter* c) {
    assert(c != NULL);
    pthread_mutex_destroy(&c->mutex);
    pthread_cond_destroy(&c->condReopen);
}

// hilo del counter 
void* hilo_counter(void* arg) {
    Counter* contador = (Counter*)arg;
    assert(contador != NULL);

    while(*(contador->activa)) {

    pthread_mutex_lock(&contador->cola->mutex);

    while (contador->cola->tam == 0 && *(contador->activa)) {
        pthread_cond_wait(&contador->cola->cond, &contador->cola->mutex);
    }

    if (contador->cola->tam == 0 && !*(contador->activa)) {
        pthread_mutex_unlock(&contador->cola->mutex);
        break;
    }

    Nodo* nodoFrente = contador->cola->cabeza;
    Pasajero* pasajero = (Pasajero*)nodoFrente->data;
    
    contador->cola->cabeza = nodoFrente->next;
    if (contador->cola->cabeza == NULL) {
        contador->cola->final = NULL;
    }
    free(nodoFrente);
    contador->cola->tam--;

    pthread_mutex_unlock(&contador->cola->mutex);

    struct timespec tiempoAtencion;
    clock_gettime(CLOCK_MONOTONIC, &tiempoAtencion);
    pasajero->tiempoAtencion = tiempoAtencion;
    pasajero->counterId = contador->id; 

    pthread_mutex_lock(&contador->mutex);
    contador->estado = COUNTER_SERVING;
    pthread_mutex_unlock(&contador->mutex);

    usleep((useconds_t)(pasajero->tiempoServicio * 1000)); 
    clock_gettime(CLOCK_MONOTONIC, &pasajero->tiempoFinAtencion);

    long tiempoEsperaMs = diferenciaMs(pasajero->tiempoLlegada, pasajero->tiempoAtencion);

    printf("Counter %d (%s) atendiendo pasajero %d (%s) | Espera: %ld ms %s\n",
    contador->id,
    nombreCounter(contador->tipo),
    pasajero->id,
    nombreClase(pasajero->clase),
    tiempoEsperaMs,
    pasajero->redirected ? "| Redirigido" : "");
    
    long tiempoServicioMs = diferenciaMs(pasajero->tiempoAtencion, pasajero->tiempoFinAtencion);

    pthread_mutex_lock(&contador->mutex);
    contador->totalAtendidos++;
    contador->sumaEsperaMs += tiempoEsperaMs;
    contador->sumaServicioMs += tiempoServicioMs;
    contador->atendidosTurno++;
    pthread_mutex_unlock(&contador->mutex);

    printf("Counter %d (%s) terminó de atender al pasajero %d (%s). Tiempo de servicio: %ld ms\n",
    contador->id,
    nombreCounter(contador->tipo),
    pasajero->id,
    nombreClase(pasajero->clase),
    tiempoServicioMs);  

    // verificar si debe entrar en break
    pthread_mutex_lock(&contador->mutex);
    int debeEntrarEnBreak = (contador->atendidosTurno >= contador->kActual) && (contador->estado == COUNTER_SERVING);
    pthread_mutex_unlock(&contador->mutex);

    if (debeEntrarEnBreak) {
        pthread_mutex_lock(&contador->mutex);
        contador->estado = COUNTER_ON_BREAK;
        int duracionBreakMs = 500 + (int)(rand_r(&contador->randSeed) % 1501);
        
        clock_gettime(CLOCK_MONOTONIC, &contador->tiempoFinBreak);
        contador->tiempoFinBreak.tv_sec += duracionBreakMs / 1000;
        contador->tiempoFinBreak.tv_nsec += (duracionBreakMs % 1000) * 1000000;

        if (contador->tiempoFinBreak.tv_nsec >= 1000000000) {
            contador->tiempoFinBreak.tv_sec += contador->tiempoFinBreak.tv_nsec / 1000000000;
            contador->tiempoFinBreak.tv_nsec = contador->tiempoFinBreak.tv_nsec % 1000000000;
        } 

        printf("Counter %d (%s) entrando en break por %d ms. Atendió %d pasajeros)\n",
            contador->id,
            nombreCounter(contador->tipo),
            duracionBreakMs,
            contador->atendidosTurno);

        // Esperar a que el Supervisor reabra el counter.
        while (contador->estado == COUNTER_ON_BREAK && *(contador->activa)) {
            pthread_cond_wait(&contador->condReopen, &contador->mutex);
        }

        // Si salimos del while porque la simulacion termino, soltamos el mutex
        // y salimos del bucle principal del counter sin reabrir.
        if (!*(contador->activa)) {
            pthread_mutex_unlock(&contador->mutex);
            break;
        }

        contador->atendidosTurno = 0;
        contador->kActual = elegirK(contador); 
        printf("Counter %d (%s) reabierto por Supervisor. Nuevo K para el siguiente turno: %d\n", contador->id, nombreCounter(contador->tipo), contador->kActual);
        pthread_mutex_unlock(&contador->mutex);
    }
    } // fin del while de la simulacion activa
    return NULL;
} 