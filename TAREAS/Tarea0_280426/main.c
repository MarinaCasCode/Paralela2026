#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>
#include <assert.h>
#include <unistd.h>
#include "counter.h"
#include "pasajero.h"
#include "queue.h"
#include "balancer.h"
#include "supervisor.h"
// compilar con: gcc -o simulacion main.c counter.c pasajero.c queue.c balancer.c supervisor.c -lpthread
// compilar en kabré con: gcc -g -std=c99 -D_POSIX_C_SOURCE=200809L -D_XOPEN_SOURCE=600 -o simulacion main.c counter.c pasajero.c queue.c balancer.c supervisor.c -lpthread
// valgrind: valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes ./simulacion 100 2 1 1 3 6 500 20
// helgrind: valgrind --tool=helgrind ./simulacion 100 2 1 1 3 6 500 20 2>&1 | tail -20

// Mutex global que protege la variable activa
pthread_mutex_t mutexActiva = PTHREAD_MUTEX_INITIALIZER; 

// FUnción para mostrar el uso del proggrama
static void verUso(const char* nombrePrograma) {
    printf("Uso: %s N mEco mBiz mIntl kMin kMax tMax umbralQ\n\n", nombrePrograma);
    printf("  N        total de pasajeros\n");
    printf("  mEco     counters Economy\n");
    printf("  mBiz     counters Business\n");
    printf("  mIntl    counters Internacional\n");
    printf("  kMin     minimo de pasajeros por turno antes de break\n");
    printf("  kMax     maximo de pasajeros por turno antes de break\n");
    printf("  tMax     tiempo maximo de espera Business antes de priority bump (ms)\n");
    printf("  umbralQ  tamaño maximo de cola antes de redistribuir a Internacional\n\n");
    printf("Ejemplo modo competencia:\n");
    printf("  %s 3000 4 2 2 3 8 500 50\n", nombrePrograma);
}

//  Elegir clase aleatoria con 70/20/10
static ClasePasajero elegirClase() {
    // El profe definió la distribución fija: 70% Economy, 20% Business, 10% Internacional
    // rand() % 100 da 0-99, se mapea así:
    // 0-69  -> Economy      (70 valores de 100 = 70%)
    // 70-89 -> Business     (20 valores de 100 = 20%)
    // 90-99 -> Internacional (10 valores de 100 = 10%)
    int dado = rand() % 100;
    if (dado < 70) return ECONOMY;
    if (dado < 90) return BUSINESS;
    return INTERNATIONAL;
}

// Encolar pasajero según su clase 
static void encolarSegunClase(Pasajero* p, Cola* colaEco, Cola* colaBiz, Cola* colaIntl) {
    switch (p->clase) {
        case ECONOMY:       encolar(colaEco,   p); break;
        case BUSINESS:      encolar(colaBiz,   p); break;
        case INTERNATIONAL: encolar(colaIntl,  p); break;
    }
}

// Suma total de todos los pasajeros atendidos en los counters, para saber el final de la simualción 
static int64_t sumarAtendidos(Counter* contadores, int numContadores) {
    int64_t total = 0;
    for (int i = 0; i < numContadores; i++) {
        // Se toma el mutex de cada counter para leer totalAtendidos de forma segura,
        // ya que los hilos de counter lo modifican concurrentemente
        pthread_mutex_lock(&contadores[i].mutex);
        total += contadores[i].totalAtendidos;
        pthread_mutex_unlock(&contadores[i].mutex);
    }
    return total;
}

// Despertar hilos bloqueados
// Se llama cuando la variable activa es 0, para que loshilos que están esperando con cond_wait se despierten y vean que la sim ha terminado 
static void despertarTodos(Cola* colaEco, Cola* colaBiz, Cola* colaIntl,
                            Counter* contadores, int numContadores) {
    // Despertar counters bloqueados esperando pasajeros en sus colas
    pthread_mutex_lock(&colaEco->mutex);
    pthread_cond_broadcast(&colaEco->cond);
    pthread_mutex_unlock(&colaEco->mutex);
 
    pthread_mutex_lock(&colaBiz->mutex);
    pthread_cond_broadcast(&colaBiz->cond);
    pthread_mutex_unlock(&colaBiz->mutex);
 
    pthread_mutex_lock(&colaIntl->mutex);
    pthread_cond_broadcast(&colaIntl->cond);
    pthread_mutex_unlock(&colaIntl->mutex);
 
    // Despertar counters que están en break esperando al Supervisor
    for (int i = 0; i < numContadores; i++) {
        pthread_mutex_lock(&contadores[i].mutex);
        pthread_cond_broadcast(&contadores[i].condReopen);
        pthread_mutex_unlock(&contadores[i].mutex);
    }
}

// main
int main(int argc, char* argv[]) {
    // Lectura y validación de args en la línea de commands
    // No hay valores hardcodeados, todo viene del usuario 
    if (argc != 9) {
        verUso(argv[0]);
        return 1;
    }
 
    int     totalPasajeros  = atoi(argv[1]);
    int32_t numEco          = (int32_t)atoi(argv[2]);
    int32_t numBiz          = (int32_t)atoi(argv[3]);
    int32_t numIntl         = (int32_t)atoi(argv[4]);
    int     kMin            = atoi(argv[5]);
    int     kMax            = atoi(argv[6]);
    int32_t tMax            = (int32_t)atoi(argv[7]);
    int32_t umbralQ         = (int32_t)atoi(argv[8]);
 
    // Validar que los valores tengan sentido
    if (totalPasajeros <= 0 || numEco < 0 || numBiz < 0 || numIntl < 0 ||
        (numEco + numBiz + numIntl) <= 0 ||
        kMin <= 0 || kMax < kMin || tMax <= 0 || umbralQ <= 0) {
        printf("Error: todos los parametros deben ser positivos y kMax >= kMin.\n");
        verUso(argv[0]);
        return 1;
    }
 
    printf("=== SIMULACION CHECK-IN AEROPUERTO ===\n");
    printf("Pasajeros=%d | Counters: Eco=%d Biz=%d Intl=%d | K=[%d,%d] | TMax=%d ms | Q=%d\n\n",
           totalPasajeros, numEco, numBiz, numIntl, kMin, kMax, tMax, umbralQ);

    setvbuf(stdout, NULL, _IOLBF, 0);
    fflush(stdout);
 
    // Semilla global para la generación de clases (solo se usa en el main, no en hilos)
    srand((unsigned int)time(NULL));

    // Crear las 3 colas 
    Cola colaEco, colaBiz, colaIntl;
    initCola(&colaEco);
    initCola(&colaBiz);
    initCola(&colaIntl);

    // Crear counters 
    // Array continuo para facilitar manejo
    int     numContadores = numEco + numBiz + numIntl;
    Counter* contadores   = (Counter*)calloc(numContadores, sizeof(Counter));
    assert(contadores != NULL);
    // Flag simulación: mientras sea 1, sigue corriendo
    // Se declara volatile para que el comp no optimice el proceso 
    volatile int activa = 1;
 
    int indice = 0;
    for (int i = 0; i < numEco;  i++, indice++)
        initCounter(&contadores[indice], indice, TIPO_ECONOMY,       &colaEco,   kMin, kMax, &activa);
    for (int i = 0; i < numBiz;  i++, indice++)
        initCounter(&contadores[indice], indice, TIPO_BUSINESS,      &colaBiz,   kMin, kMax, &activa);
    for (int i = 0; i < numIntl; i++, indice++)
        initCounter(&contadores[indice], indice, TIPO_INTERNATIONAL, &colaIntl,  kMin, kMax, &activa);

    // Crear supervisor y balancer 
    Supervisor supervisor;
    initSupervisor(&supervisor, contadores, numContadores, &activa);
 
    Balancer balancer;
    initBalancer(&balancer, &colaEco, &colaBiz, &colaIntl, umbralQ, tMax, &activa,
                 numEco, numBiz, numIntl);

    // Crear y encolar pasaejros antes de lanzar los hilos a acomodar la llegada de pasajeros al aeropuerto, y para que haya pasajeros en las colas desde el inicio y así los counters no se queden sin hacer nada al principio
    // Ya que el profe indicó que todos los pasajeros llegan desde el inicio
    Pasajero** pasajeros = (Pasajero**)calloc(totalPasajeros, sizeof(Pasajero*));
    assert(pasajeros != NULL);
 
    for (int i = 0; i < totalPasajeros; i++) {
        ClasePasajero clase = elegirClase(); // 70/20/10
        // Pasamos 0 como tiempoServicio para que crearPasajero genere uno aleatorio en [10,100] ms
        pasajeros[i] = crearPasajero(i, clase, 0);
        // Refrescar tiempoLlegada al momento real de encolado
        clock_gettime(CLOCK_MONOTONIC, &pasajeros[i]->tiempoLlegada);
        encolarSegunClase(pasajeros[i], &colaEco, &colaBiz, &colaIntl);
    }
 
    printf("\nPasajeros creados y en cola. Iniciando simulacion...\n\n");

    // Registrar timepo de inicio 
    struct timespec tiempoInicio, tiempoFin;
    clock_gettime(CLOCK_MONOTONIC, &tiempoInicio);

    // Lanzar hilos de counters, supervisor y balancer
    pthread_t* hilosCounter = (pthread_t*)calloc(numContadores, sizeof(pthread_t));
    assert(hilosCounter != NULL);
 
    for (int i = 0; i < numContadores; i++)
        pthread_create(&hilosCounter[i], NULL, hiloCounter, &contadores[i]);
 
    pthread_t hiloSup, hiloBal;
    pthread_create(&hiloSup, NULL, hiloSupervisor, &supervisor);
    pthread_create(&hiloBal, NULL, hiloBalancer,   &balancer);

    // Esperar a que pasajeros sean atenddidos, se revisa cada 50ms con nano sleep
      while (sumarAtendidos(contadores, numContadores) < (int64_t)totalPasajeros) {
        struct timespec espera = {0, 50 * 1000000L}; // 50 ms
        nanosleep(&espera, NULL);
    }

    // Registrar tiempo de fin y calcualr la duración 
    clock_gettime(CLOCK_MONOTONIC, &tiempoFin);

    // "Apagar simulación" y despertar a los hilos 
    clock_gettime(CLOCK_MONOTONIC, &tiempoFin);
 
    // Poner activa en 0 para que los hilos vean que la simulación terminó, con mutex para sincronización   
    pthread_mutex_lock(&mutexActiva);
    activa = 0;
    pthread_mutex_unlock(&mutexActiva);
    despertarTodos(&colaEco, &colaBiz, &colaIntl, contadores, numContadores);

    // Esperar a que hilos terminen
    for (int i = 0; i < numContadores; i++)
        pthread_join(hilosCounter[i], NULL);
    pthread_join(hiloSup, NULL);
    pthread_join(hiloBal, NULL);

    // Estadísticas
        long tiempoTotalMs = diferenciaMs(tiempoInicio, tiempoFin);
 
    // Contar atendidos por clase y total de redirected
    int64_t porClase[3]    = {0, 0, 0};
    int64_t totalRedirigidos = 0;
    for (int i = 0; i < totalPasajeros; i++) {
        if (pasajeros[i]->counterId >= 0)
            porClase[pasajeros[i]->clase]++;
        if (pasajeros[i]->redirected)
            totalRedirigidos++;
    }
 
    printf("\n══════════════════════════════════════════\n");
    printf("           ESTADÍSTICAS FINALES\n");
    printf("══════════════════════════════════════════\n");
    printf("Tiempo total de ejecucion : %ld ms\n", tiempoTotalMs);
    printf("Pasajeros atendidos       : %d / %d\n",
           (int)(porClase[0] + porClase[1] + porClase[2]), totalPasajeros);
    printf("  Economy       : %lld\n", (long long)porClase[ECONOMY]);
    printf("  Business      : %lld\n", (long long)porClase[BUSINESS]);
    printf("  Internacional : %lld\n", (long long)porClase[INTERNATIONAL]);
    printf("Redirigidos (bumps + overflow): %lld\n\n", (long long)totalRedirigidos);
 
    printf("--- Desglose por counter ---\n");
    int64_t sumaEsperaGlobal   = 0;
    int64_t sumaServicioGlobal = 0;
    int64_t totalAtendidosGlobal = 0;
 
    for (int i = 0; i < numContadores; i++) {
        const char* tipo = (contadores[i].tipo == TIPO_ECONOMY)       ? "Economy" :
                           (contadores[i].tipo == TIPO_BUSINESS)      ? "Business" :
                                                                         "Internacional";
        long long atendidos = (long long)contadores[i].totalAtendidos;
        double promedioEspera   = atendidos > 0 ? (double)contadores[i].sumaEsperaMs   / atendidos : 0.0;
        double promedioServicio = atendidos > 0 ? (double)contadores[i].sumaServicioMs / atendidos : 0.0;
 
        printf("Counter %d (%-13s): atendidos=%-4lld espera prom=%.1f ms  servicio prom=%.1f ms\n",
               contadores[i].id, tipo, atendidos, promedioEspera, promedioServicio);
 
        sumaEsperaGlobal    += contadores[i].sumaEsperaMs;
        sumaServicioGlobal  += contadores[i].sumaServicioMs;
        totalAtendidosGlobal += contadores[i].totalAtendidos;
    }
 
    if (totalAtendidosGlobal > 0) {
        printf("\nEspera promedio global  : %.1f ms\n", (double)sumaEsperaGlobal   / totalAtendidosGlobal);
        printf("Servicio promedio global: %.1f ms\n",   (double)sumaServicioGlobal / totalAtendidosGlobal);
    }
    printf("══════════════════════════════════════════\n");
 

    // Liberar memoria 
    // Va a ser liberado de orden inverso a como se creó para evitar liberar algo que todavía se está usando
        for (int i = 0; i < totalPasajeros; i++)
        destruirPasajero(pasajeros[i]);
    free(pasajeros);
 
    for (int i = 0; i < numContadores; i++)
        destruirCounter(&contadores[i]);
    free(contadores);
    free(hilosCounter);
 
    destruirBalancer(&balancer);
    destruirSupervisor(&supervisor);
    destruirCola(&colaEco);
    destruirCola(&colaBiz);
    destruirCola(&colaIntl);
 
    pthread_mutex_destroy(&mutexActiva);

    return 0;
}