#include "balancer.h"
#include "pasajero.h"
#include <stdio.h> // para printf
#include <assert.h> // para assert que detiene el programa si se trata de hacer algo
#include <time.h> // para clock_gettime y nanosleep
#include <stdlib.h> // para malloc y free

extern pthread_mutex_t mutexActiva; 

//Init, inicializa balancer 
void initBalancer(Balancer* b, Cola* colaEconomy, Cola* colaBusiness, Cola* colaInternacional,
                  int32_t maxEnCola, int32_t tMaxEspera, volatile int* activa,
                  int32_t numCountersEco, int32_t numCountersBiz, int32_t numCountersIntl){
    assert(b != NULL); 
    assert(colaEconomy != NULL); 
    assert(colaBusiness != NULL); 
    assert(colaInternacional != NULL); 
    assert(activa != NULL); 

    b->colaEconomy = colaEconomy;
    b->colaBusiness = colaBusiness; 
    b->colaInternacional = colaInternacional; 
    b->maxEnCola = maxEnCola;
    b->tMaxEspera = tMaxEspera;
    b->activa = activa; 

    // Conteo de counters por clase, para saber si hay que vaciar agresivamente.
    b->numCountersEco = numCountersEco;
    b->numCountersBiz = numCountersBiz;
    b->numCountersIntl = numCountersIntl;

    // Anti-starvation: arranca con 0 bumps y la ventana inicia AHORA.
    b->bumpsEnVentana = 0;
    clock_gettime(CLOCK_MONOTONIC, &b->inicioVentana);

    printf("Balancer inicializado, máxima cantidad de pasajeros que va a permitir en la cola: %d, tiempo límite de espera para clase Business: %d ms\n", maxEnCola, tMaxEspera);   
}

// destructor 
void destruirBalancer(Balancer* b) {
    assert(b != NULL); 
    b->colaEconomy = NULL;
    b->colaBusiness = NULL;
    b->colaInternacional = NULL;
    b->maxEnCola = 0;
    b->tMaxEspera = 0;
    b->activa = NULL; 
    printf("Balancer destruido.\n");
}

// Ver si se hace priority bump, función privada 
static void revisarPriorityBump(Balancer* b) {
    const int32_t MAX_BUMPS_POR_VENTANA = 3;
    const long    VENTANA_MS            = 1000;

    struct timespec ahora;
    clock_gettime(CLOCK_MONOTONIC, &ahora);

    long msDesdeInicio = diferenciaMs(b->inicioVentana, ahora);
    if (msDesdeInicio >= VENTANA_MS) {
        b->bumpsEnVentana = 0;
        b->inicioVentana = ahora;
    }

    // Si ya se alcanzo el cap en esta ventana, no se hacen mas bumps por ahora.
    // EXCEPCION: si no hay counters Business, todos los pasajeros Business deben
    // pasar a Internacional (si no, se quedan atrapados sin nadie que los atienda).
    if (b->bumpsEnVentana >= MAX_BUMPS_POR_VENTANA && b->numCountersBiz > 0) {
        return;
    }

    pthread_mutex_lock(&b->colaBusiness->mutex);
    Nodo* anterior = NULL; 
    Nodo* actual = b->colaBusiness->cabeza;

    while (actual != NULL) {
        Pasajero* pasajero = (Pasajero*)actual->data;
        long espera = diferenciaMs(pasajero->tiempoLlegada, ahora);

        // Si no hay counters Business, bumpear sin esperar a tMaxEspera
        long umbralEspera = (b->numCountersBiz == 0) ? 0 : b->tMaxEspera;
        if (espera >= umbralEspera) {
            printf("Balancer: Pasajero %d en Business ha estado esperando %ld ms, superando el límite de %d ms. Haciendo priority bump a International.\n", pasajero->id, espera, b->tMaxEspera);
            if (anterior == NULL) {
                b->colaBusiness->cabeza = actual->next;
            } else {
                anterior->next = actual->next;
            }
            if (actual->next == NULL) {
                b->colaBusiness->final = anterior; 
            }
            void* data = actual->data;
            free(actual);
            b->colaBusiness->tam--;
            
            pthread_mutex_unlock(&b->colaBusiness->mutex);

            pasajero->redirected = 1;
            encolarAlFrente(b->colaInternacional, data);
            printf("Balancer: PRIORITY BUMP: Pasajero %d movido al frente de cola Internacional, por llevar esperando %ld ms. \n", pasajero->id, espera);

            b->bumpsEnVentana++;
            return;
        }
        anterior = actual;
        actual = actual->next;
    }
    pthread_mutex_unlock(&b->colaBusiness->mutex);
}

// Redistribucion por umbral Q.
static void redistribuirCola(Cola* origen, Cola* destino, int32_t maxEnCola, const char* nombreOrigen) {
    assert(origen != NULL);
    assert(destino != NULL);

    pthread_mutex_lock(&origen->mutex);

    if ((int32_t)origen->tam <= maxEnCola) {
        pthread_mutex_unlock(&origen->mutex);
        return;
    }

    int32_t excedente = (int32_t)origen->tam - maxEnCola;

    Pasajero** aMover = (Pasajero**)malloc(sizeof(Pasajero*) * excedente);
    assert(aMover != NULL);

    size_t corte = origen->tam - (size_t)excedente;
    Nodo* prevCorte = NULL;
    Nodo* actual = origen->cabeza;
    for (size_t i = 0; i < corte && actual != NULL; i++) {
        prevCorte = actual;
        actual = actual->next;
    }

    int32_t idx = 0;
    while (actual != NULL && idx < excedente) {
        aMover[idx++] = (Pasajero*)actual->data;
        Nodo* sig = actual->next;
        free(actual);
        actual = sig;
    }

    if (prevCorte != NULL) {
        prevCorte->next = NULL;
        origen->final = prevCorte;
    } else {
        origen->cabeza = NULL;
        origen->final = NULL;
    }
    origen->tam = corte;

    pthread_mutex_unlock(&origen->mutex);

    for (int32_t i = 0; i < excedente; i++) {
        aMover[i]->redirected = 1;
        encolar(destino, aMover[i]);
        printf("Balancer: REDISTRIBUCION: Pasajero %d movido de %s a Internacional (cola %s excedia umbral %d).\n",
               aMover[i]->id, nombreOrigen, nombreOrigen, maxEnCola);
    }

    free(aMover);
}

// hilo principal de balancer 
void* hiloBalancer(void* arg) {
    Balancer* balancer = (Balancer*)arg;
    assert(balancer != NULL);

    const long INTERVALO_BALANCER_MS = 50;

    printf("Hilo balancer iniciado. Intervalo de revision: %ld ms.\n", INTERVALO_BALANCER_MS);

    pthread_mutex_lock(&mutexActiva);
    int simActiva = *(balancer->activa);
    pthread_mutex_unlock(&mutexActiva);
    while (simActiva) {
        revisarPriorityBump(balancer);

        int32_t umbralEco = (balancer->numCountersEco == 0) ? 0 : balancer->maxEnCola;
        int32_t umbralBiz = (balancer->numCountersBiz == 0) ? 0 : balancer->maxEnCola;
        redistribuirCola(balancer->colaEconomy, balancer->colaInternacional,
                         umbralEco, "Economy");
        redistribuirCola(balancer->colaBusiness, balancer->colaInternacional,
                         umbralBiz, "Business");

        pthread_mutex_lock(&mutexActiva);
        simActiva = *(balancer->activa);
        pthread_mutex_unlock(&mutexActiva);

        struct timespec espera;
        espera.tv_sec  = INTERVALO_BALANCER_MS / 1000;
        espera.tv_nsec = (INTERVALO_BALANCER_MS % 1000) * 1000000L;
        nanosleep(&espera, NULL);
    }

    printf("Hilo balancer terminando, simulacion inactiva.\n");
    return NULL;
}