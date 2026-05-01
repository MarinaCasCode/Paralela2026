#include <stdio.h>
#include <stdlib.h>
#include "counter.h"
#include "pasajero.h"
#include "queue.h"
#include <unistd.h> // para sleep

int main() {
    volatile int activa = 1;

    // 1. Crear la cola
    Cola colaEconomy;
    initCola(&colaEconomy);

    // 2. Crear el counter
    Counter contador;
    initCounter(&contador, 1, TIPO_ECONOMY, &colaEconomy, 2, 4, &activa);

    // 3. Meter algunos pasajeros de prueba
    for (int i = 0; i < 5; i++) {
        Pasajero* p = crear_pasajero(i, ECONOMY, 0);
        encolar(&colaEconomy, p);
    }

    // 4. Lanzar el hilo del counter
    pthread_t hilo;
    pthread_create(&hilo, NULL, hilo_counter, &contador);

    // 5. Esperar un poco y apagar la simulación
    sleep(3);
    activa = 0;
    pthread_cond_broadcast(&colaEconomy.cond); // despertar el hilo para que vea activa=0

    pthread_join(hilo, NULL);

    // 6. Limpiar
    destruirCounter(&contador);
    destruirCola(&colaEconomy);

    printf("\nPrueba terminada. Atendidos: %lld\n", (long long)contador.totalAtendidos);
    return 0;
}