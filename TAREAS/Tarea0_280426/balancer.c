#include "balancer.h"
#include "pasajero.h"
#include <stdio.h> // para printf
#include <assert.h> // para assert que detiene el programa si se trata de hacer algo
#include <time.h> // para clock_gettime y nanosleep
#include <stdlib.h> // para malloc y free

// priority bump: inserta un pasajero al frente de la cola, se usa para el balancer cuando un pasajero lleva mucho tiempo esperando en business y se decide subirlo a internacional para que sea atendido más rápido
void priorityBump(Cola* cola, void* data) {
    assert(cola != NULL); // Asegura que la cola no sea nula
    // se crea nodo nodo antes de mutex lock porque no tiene sentido bloquear la cola para crear un nodo, y así se evita bloquear la cola por más tiempo del necesario
    Nodo* nuevoNodo = (Nodo*)malloc(sizeof(Nodo)); // Nuevo nodo para el pasajero que se va a insertar
    nuevoNodo->data = data; // Asigna los datos al nuevo nodo
    nuevoNodo->next = NULL; // Siguiente nodo es NULL

    pthread_mutex_lock(&cola->mutex); // Bloquea mutex para modificar la cola

    if (cola->cabeza == NULL) {
        // Si la cola está vacía, el nuevo nodo es la cabeza y el final
        cola->cabeza = nuevoNodo;
        cola->final = nuevoNodo;
    } else {
        // Si la cola no está vacía, inserta el nuevo nodo al frente
        nuevoNodo->next = cola->cabeza; // El siguiente del nuevo nodo es la actual cabeza
        cola->cabeza = nuevoNodo; // El nuevo nodo se convierte en la nueva cabeza
    }
    cola->tam++; // Se incrementa tam de la cola 
    pthread_cond_signal(&cola->cond); // Avisa a ualquier hilo esperando que hay nuevo elemento
    pthread_mutex_unlock(&cola->mutex); // Desbloquea mutex después de modificar la cola
}

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
static void revisarPriorityBump(Balancer* b) {
    struct timespec ahora;
    clock_gettime(CLOCK_MONOTONIC, &ahora); // obtiene el tiempo actual para comparar con el tiempo de atención de los pasajeros en business y hacer priority bump si es necesario

    pthread_mutex_lock(&b->colaBusiness->mutex); // Bloquea mutex de la cola de business para revisar su tamaño y tiempos de espera de sus pasajeros
    Nodo* anterior = NULL; 
    Nodo* actual = b->colaBusiness->cabeza;

    while (actual != NULL) {
        Pasajero* pasajero = (Pasajero*)actual->data; // Obtiene pasajero del nodo 
        long espera = calcularDiferenciaMs(pasajero->tiempoLlegada, ahora); // Calcula el tiempo de espera del pasajero business desde que lleggó hasta ahora 

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
            return; // Se hace un priority bump a la vez para evitar starvation
        }
        anterior = actual; // Avanza el nodo anterior al actual
        actual = actual->next; // Avanza al siguiente nodo
    }
    pthread_mutex_unlock(&b->colaBusiness->mutex); // Desbloquea mutex de business después de revisar todos los pasajeros
}

// hilo principal de balancer 
void* hiloBalancer(void* arg) {
    Balancer* balancer = (Balancer*)arg; // Convierte argumento a un puntero a Balancer
    assert(balancer != NULL); // Asegura que el balancer no sea nulo

}
}