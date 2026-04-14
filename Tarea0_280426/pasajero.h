#ifndef PASAJERO_H
#define PASAJERO_H

#include <time.h> // para time_t
#include <stdint.h> // para unsiigned int de 32 bits, mejor que int para IDs

typedef enum {
    ECONOMY = 0,
    BUSINESS = 1,
    FIRST_CLASS = 2
} ClasePasajero;

typedef struct { // tdo lo que se necesita para representar a un pasajero
   int32_t id; // ID único del pasajero
    ClasePasajero clase; // Clase del pasajero

    // tiempo de service
    int32_t tiempoServicio; // Tiempo que el pasajero tarda en ser atendido (en segundos)

    // timestamps
    struct timespec tiempoLlegada; // Tiempo de llegada del pasajero
    struct timespec tiempoAtencion; // Tiempo en que el pasajero comienza a ser atendido
    struct timespec tiempoFinAtencion; // Tiempo en que el pasajero termina de ser atendido

    // resultado de check in
    int32_t counterId; // ID del counter que atendió al pasajero
    int prioridad; // Prioridad del pasajero, calculada a partir de su clase y tiempo de espera
    int redirected; // Indica si el pasajero fue redirigido a otro counter (1 si fue redirigido, 0 si no)

    // crear pasajero
    //liberar memo de pasajero
    // devolucion de el nombre de la clase del pasajero 
    // tiempo entre dos procesos 
} Pasajero;

#endif // PASAJERO_H