#include "balancer.h"
#include "pasajero.h"
#include <stdio.h> // para printf
#include <assert.h> // para assert que detiene el programa si se trata de hacer algo
#include <time.h> // para clock_gettime y nanosleep
#include <stdlib.h> // para malloc y free

extern pthread_mutex_t mutexActiva; 

//Init, inicializa balancer 
void initBalancer(Balancer* b, Cola* colaEconomy, Cola* colaBusiness, Cola* colaInternacional, int32_t maxEnCola, int32_t tMaxEspera, volatile int* activa){
    assert(b != NULL); 
    assert(colaEconomy != NULL); 
    assert(colaBusiness != NULL); 
    assert(colaInternacional != NULL); 
    assert(activa != NULL); 

    b->colaEconomy = colaEconomy; // Asigna la cola de economy al balancer
    b->colaBusiness = colaBusiness; 
    b->colaInternacional = colaInternacional; 
    b->maxEnCola = maxEnCola;
    b->tMaxEspera = tMaxEspera;
    b->activa = activa; 

    // Anti-starvation: arranca con 0 bumps y la ventana inicia AHORA.
    b->bumpsEnVentana = 0;
    clock_gettime(CLOCK_MONOTONIC, &b->inicioVentana);

    printf("Balancer inicializado, máxima cantidad de pasajeros que va a permitir en la cola: %d, tiempo límite de espera para clase Business: %d ms\n", maxEnCola, tMaxEspera);   
}

// destructor 
void destruirBalancer(Balancer* b) {
    assert(b != NULL); 
    // Como balancer no tiene memoria propia, solo guarda punteros a las colas y al flag de simulacion, no se libera memoria de las colas ni del flag, solo se limpia struct por si se quiere reutilizar
    b->colaEconomy = NULL;
    b->colaBusiness = NULL;
    b->colaInternacional = NULL;
    b->maxEnCola = 0;
    b->tMaxEspera = 0;
    b->activa = NULL; 
    printf("Balancer destruido.\n");
}

// Ver si se hace priority bump, función privada 
// Si alguna cola supera umbral de cantidad de pasajeros, es pasado a internacional
// Se hace cola por cola para evitar hacer priority bumps, porque si se hacen todos al mismo tiempo puede que haya una cola en la que ya no se ocupaba hacer por el movimiento que hicimos con una 
// Mutex para revisar colas y hacer priority bumps son 100% necesarios o si no ocurriría un deadlock, porque el hilo del balancer necesita revisar el tamaño de las colas para decidir si hace priority bump, y el hilo de los counters también necesita revisar el tamaño de las colas para saber si debe esperar o no
//
// Anti-starvation: maximo MAX_BUMPS_POR_VENTANA bumps por ventana de VENTANA_MS.
static void revisarPriorityBump(Balancer* b) {
    // Limites de la cuota anti-starvation.
    const int32_t MAX_BUMPS_POR_VENTANA = 3;
    const long    VENTANA_MS            = 1000;

    struct timespec ahora;
    clock_gettime(CLOCK_MONOTONIC, &ahora); // obtiene el tiempo actual para comparar con el tiempo de atención de los pasajeros en business y hacer priority bump si es necesario

    // Reset de ventana si ya paso VENTANA_MS desde su inicio.
    long msDesdeInicio = diferenciaMs(b->inicioVentana, ahora);
    if (msDesdeInicio >= VENTANA_MS) {
        b->bumpsEnVentana = 0;
        b->inicioVentana = ahora;
    }

    // Si ya se alcanzo el cap en esta ventana, no se hacen mas bumps por ahora.
    if (b->bumpsEnVentana >= MAX_BUMPS_POR_VENTANA) {
        return;
    }

    pthread_mutex_lock(&b->colaBusiness->mutex); // Bloquea mutex de la cola de business para revisar su tamaño y tiempos de espera de sus pasajeros
    Nodo* anterior = NULL; 
    Nodo* actual = b->colaBusiness->cabeza;

    while (actual != NULL) {
        Pasajero* pasajero = (Pasajero*)actual->data; // Obtiene pasajero del nodo 
        long espera = diferenciaMs(pasajero->tiempoLlegada, ahora); // Calcula el tiempo de espera del pasajero business desde que lleggó hasta ahora 

        if (espera >= b->tMaxEspera) { // Si tiempo de espera que lelva es mayor que el límite se hace priority bump
            printf("Balancer: Pasajero %d en Business ha estado esperando %ld ms, superando el límite de %d ms. Haciendo priority bump a International.\n", pasajero->id, espera, b->tMaxEspera);
            // Desencolar pasajero d ela cola bussiness
            if (anterior == NULL) {
                // El pasajero a mover es el primero de la cola
                b->colaBusiness->cabeza = actual->next; // Actualiza la cabeza de la cola de business al siguiente nodo
            } else {
                // El pasajero a mover no es el primero, se salta el nodo actual
                anterior->next = actual->next; // El nodo anterior ahora apunta al siguiente del actual, removiendo al actual de la cola de business
            }
            if (actual->next == NULL) {
                // Si el nodo actual es el último, se actualiza el final de la cola de business
                b->colaBusiness->final = anterior; 
            }
            void* data = actual->data; // Guarda los datos del nodo actual para hacer priority bump
            free(actual); // Libera el nodo actual que ya fue removido de la cola de
            b->colaBusiness->tam--; // Se decrementa el tamaño de la cola de business
            
            pthread_mutex_unlock(&b->colaBusiness->mutex); // Desbloquea mutex de la cola de business antes de hacer priority bump para evitar bloquear la cola por más tiempo del que se necesita 

            // Marcar a pasajero para redirigirlo 
            pasajero->redirected = 1; // Para estadísticas finales
            
            // Pasar como primero a internacional 
            // Función privada ya que solo balancer la puede usar 
            encolarAlFrente(b->colaInternacional, data); // Inserta al pasajero al frente de la cola internacional para que sea atendido lo más pronto posible
            printf("Balancer: PRIORITY BUMP: Pasajero %d movido al frente de cola Internacional, por llevar esperando %ld ms. \n", pasajero->id, espera);

            // Contabilizar el bump para la cuota anti-starvation.
            b->bumpsEnVentana++;

            return; // Se hace un priority bump a la vez para evitar starvation
        }
        anterior = actual; // Avanza el nodo anterior al actual
        actual = actual->next; // Avanza al siguiente nodo
    }
    pthread_mutex_unlock(&b->colaBusiness->mutex); // Desbloquea mutex de business después de revisar todos los pasajeros
}

// Redistribucion por umbral Q.
static void redistribuirCola(Cola* origen, Cola* destino, int32_t maxEnCola, const char* nombreOrigen) {
    assert(origen != NULL);
    assert(destino != NULL);

    // extraer excedente de origen
    pthread_mutex_lock(&origen->mutex);

    if ((int32_t)origen->tam <= maxEnCola) {
        // No hay excedente, salimos rapido sin hacer nada.
        pthread_mutex_unlock(&origen->mutex);
        return;
    }

    int32_t excedente = (int32_t)origen->tam - maxEnCola;

    // Lista local de pasajeros a mover
    Pasajero** aMover = (Pasajero**)malloc(sizeof(Pasajero*) * excedente);
    assert(aMover != NULL);

    size_t corte = origen->tam - (size_t)excedente; // cantidad de nodos que se quedan
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

    // Cortar la lista de origen
    if (prevCorte != NULL) {
        prevCorte->next = NULL;
        origen->final = prevCorte;
    } else {
        // corte == 0: la cola queda vacia
        origen->cabeza = NULL;
        origen->final = NULL;
    }
    origen->tam = corte;

    pthread_mutex_unlock(&origen->mutex);

    // encolar en destino
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
    Balancer* balancer = (Balancer*)arg; // Convierte argumento a un puntero a Balancer
    assert(balancer != NULL); // Asegura que el balancer no sea nulo

    // Intervalo de polling del balancer en milisegundos.
    const long INTERVALO_BALANCER_MS = 50;

    printf("Hilo balancer iniciado. Intervalo de revision: %ld ms.\n", INTERVALO_BALANCER_MS);

    pthread_mutex_lock(&mutexActiva);
    int simActiva = *(balancer->activa);
    pthread_mutex_unlock(&mutexActiva);
    while (simActiva) {
        // 1. Priority bump: business que han esperado mas que tMaxEspera -> frente de Internacional.
        revisarPriorityBump(balancer);

        // 2. Redistribucion por umbral Q: si Economy o Business exceden maxEnCola,
        //    mover excedente al final de Internacional.
        redistribuirCola(balancer->colaEconomy, balancer->colaInternacional,
                         balancer->maxEnCola, "Economy");
        redistribuirCola(balancer->colaBusiness, balancer->colaInternacional,
                         balancer->maxEnCola, "Business");

    pthread_mutex_lock(&mutexActiva);
        simActiva = *(balancer->activa);
        pthread_mutex_unlock(&mutexActiva);

        // Pausa antes de la siguiente revision para no consumir CPU innecesariamente.
        struct timespec espera;
        espera.tv_sec  = INTERVALO_BALANCER_MS / 1000;
        espera.tv_nsec = (INTERVALO_BALANCER_MS % 1000) * 1000000L;
        nanosleep(&espera, NULL);
    }

    printf("Hilo balancer terminando, simulacion inactiva.\n");
    return NULL;
}