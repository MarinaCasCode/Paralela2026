#include "pasajero.h"
#include <stdlib.h> // para malloc y free
#include <assert.h> // para assert que detiene el programa si se trata de hacer algo "imposible" como destruir un pasajero nulo

Pasajero* crear_pasajero(int32_t id, ClasePasajero clase, int32_t tiempoServicio) {
    Pasajero* p = (Pasajero*)malloc(sizeof(Pasajero)); // Crea un nuevo pasajero pidiendo su tamaño en cantidad de memoria
    p->id = id; // Asigna el ID al pasajero
    p->clase = clase; // Asigna la clase al pasajero
    // rango de 10-100 ms para el tiempo de servicio, se asigna aleatoriamente para simular la variabilidad en el tiempo que tarda cada pasajero en ser atendido
    p->tiempoServicio = 10 + rand() % 11; // Asigna un tiempo de servicio aleatorio entre 10 y 20 segundos
    clock_gettime(CLOCK_MONOTONIC, &p->tiempoLlegada); // Registra el tiempo de llegada del pasajero

    // cuando el counter lo atienda se llena
    p->tiempoAtencion = (struct timespec){0, 0}; // Inicializa el tiempo de atención a 0
    p->tiempoFinAtencion = (struct timespec){0, 0}; // Inicializa el tiempo de fin de atención a 0
    p->counterId = -1; // Inicializa el ID del counter que atendió al pasajero a -1 (no atendido)
    p->prioridad = 0; // Inicializa la prioridad del pasajero a 0 (sin prioridad)
    p->redirected = 0; // Inicializa el flag de redirección a 0 (no redirigido)
    return p; // Devuelve el nuevo pasajero creado
}

void destruir_pasajero(Pasajero* p) {
    assert(p != NULL); // Asegura que el pasajero no sea nulo
    free(p); // Libera la memoria asignada al pasajero
}

const char* nombre_clase(ClasePasajero clase) {
    switch (clase) { // Devuelve el nombre de la clase del pasajero según su valor
        case ECONOMY:
            return "Economy";
        case BUSINESS:
            return "Business";
        case INTERNATIONAL:
            return "International";
        default:
            return "Unknown"; // Si la clase no es reconocida, devuelve "Unknown"
    }
}

long diferenciaMs(struct timespec inicio, struct timespec fin) {
    long segundos = fin.tv_sec - inicio.tv_sec; // Calcula la diferencia en segundos
    long nanosegundos = fin.tv_nsec - inicio.tv_nsec; // Calcula la diferencia en nanosegundos para obtener una mayor precisión
    return segundos * 1000 + nanosegundos / 1000000; // Convierte la diferencia total a milisegundos y la devuelve
}
