#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "counter.h"
#include "pasajero.h"
#include "queue.h"
#include "supervisor.h"

// compilar con gcc counter.c pasajero.c queue.c supervisor.c testCounter.c -o testCounter -lpthread

int main() {
    volatile int activa = 1;

    // 1. Crear la cola
    Cola colaEconomy;
    initCola(&colaEconomy);

    // 2. Crear el counter
    Counter contador;
    initCounter(&contador, 1, TIPO_ECONOMY, &colaEconomy, 2, 4, &activa);

    // 3. Crear el supervisor con el arreglo de counters
    Supervisor supervisor;
    initSupervisor(&supervisor, &contador, 1, &activa);

    // 4. Meter pasajeros de prueba
    for (int i = 0; i < 20; i++) {
        Pasajero* p = crearPasajero(i, ECONOMY, 0);
        encolar(&colaEconomy, p);
    }

    // 5. Lanzar hilo del counter y del supervisor
    pthread_t hiloCounter, hiloSup;
    pthread_create(&hiloCounter, NULL, hiloCounter, &contador);
    pthread_create(&hiloSup,     NULL, hiloSupervisor, &supervisor);

    // 6. Esperar a que terminen todos los pasajeros y apagar
    sleep(15);
    activa = 0;
    pthread_cond_broadcast(&colaEconomy.cond);

    pthread_join(hiloCounter, NULL);
    pthread_join(hiloSup,     NULL);

    // 7. Limpiar
    destruirSupervisor(&supervisor);
    destruirCounter(&contador);
    destruirCola(&colaEconomy);

    printf("\nPrueba terminada. Atendidos: %lld\n", (long long)contador.totalAtendidos);
    return 0;
}