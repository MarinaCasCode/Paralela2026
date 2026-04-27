#ifndef PASAJERO_H
#define PASAJERO_H

#include <time.h> // para time_t
#include <stdint.h> // para unsiigned int de 32 bits, mejor que int para IDs

typedef enum {
    ECONOMY = 0,
    BUSINESS = 1,
    INTERNATIONAL = 2
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
    int prioridad; // Prioridad del pasajero, calculada a partir de su clase y tiempo de espera. Si lleva mucho haciendo fila el balnacer debe toamrlo y lleavrlo al frente de la cola internacional 
    int redirected; // Indica si el pasajero fue redirigido a otro counter (1 si fue redirigido, 0 si no)

} Pasajero;

// crear pasajero
Pasajero* crear_pasajero(int32_t id, ClasePasajero clase, int32_t tiempoServicio);
//liberar memo de pasajero
void destruir_pasajero(Pasajero* p);
// devolucion de el nombre de la clase del pasajero 
const char* nombre_clase(ClasePasajero clase);
// tiempo entre dos procesos 
long diferenciaMs(struct timespec inicio, struct timespec fin); // se usa para que balancer vea cuanto tiempo lleva esperando un pasajero y así pueda decidir si redirigirlo o no

#endif // PASAJERO_H