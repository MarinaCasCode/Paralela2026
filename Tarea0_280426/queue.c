#include "queue.h"
#include <stdlib.h> // para malloc y free
#include <assert.h> // para assert que detiene el programa si se trata de hacer algo "imposible" como destruir una cola nula

void initCola(Cola* cola) {
    assert(cola != NULL); // Asegura que la cola no sea nula
    cola->cabeza = NULL; // Inicializa la cabeza a NULL
    cola->final = NULL;  // Inicializa el final a NULL
    cola->tam = 0;       // Inicializa el tamaño a 0
    pthread_mutex_init(&cola->mutex, NULL); // Inicializa el mutex
    pthread_cond_init(&cola->cond, NULL);   // Inicializa la variable de condición
}

void destruirCola(Cola* cola) {
    assert(cola != NULL); // Asegura que la cola no sea nula
    pthread_mutex_lock(&cola->mutex); // Bloquea el mutex para evitar modificaciones concurrentes
    Nodo* actual = cola->cabeza; // Comienza desde la cabeza de la cola

    while (actual != NULL) { // Recorre la lista de nodos
        Nodo* temp = actual; // Guarda el nodo actual
        actual = actual->next; // Avanza al siguiente nodo
        free(temp); // Libera el nodo actual
    }
    pthread_mutex_unlock(&cola->mutex); // Desbloquea el mutex
    pthread_mutex_destroy(&cola->mutex); // Destruye el mutex
    pthread_cond_destroy(&cola->cond);   // Destruye la variable de condición
}

void encolar(Cola* cola, void* data) {
    // Se pide memoria antes de usar el mutex ya que no tiene sentido bloquear la cola para crear un nodo, y así se evita bloquear la cola por más tiempo del necesario
    assert(cola != NULL); // Asegura que la cola no sea nula
    Nodo* nuevoNodo = (Nodo*)malloc(sizeof(Nodo)); // Crea un nuevo nodo pidiendo su tamaño en cantidad de memoria
    nuevoNodo->data = data; // Asigna los datos al nuevo nodo
    nuevoNodo->next = NULL; // El siguiente nodo es NULL

    pthread_mutex_lock(&cola->mutex); // Bloquea el mutex para modificar la cola
    if (cola->final == NULL) { // Si la cola está vacía
        cola->cabeza = nuevoNodo; // El nuevo nodo es la cabeza
        cola->final = nuevoNodo;  // El nuevo nodo también es el final
    } else {
        cola->final->next = nuevoNodo; // Enlaza el nuevo nodo al final de la cola
        cola->final = nuevoNodo; // Actualiza el final de la cola
    }
    cola->tam++; // Incrementa el tamaño de la cola
    pthread_cond_signal(&cola->cond); // Señala a cualquier hilo esperando que hay un nuevo elemento
    pthread_mutex_unlock(&cola->mutex); // Desbloquea el mutex
}

void* desencolar(Cola* cola) {
    assert(cola != NULL); // Asegura que la cola no sea nula
    pthread_mutex_lock(&cola->mutex); // Bloquea el mutex para modificar la cola

    while (cola->tam == 0) { // Si la cola está vacía, espera a que haya un elemento
        pthread_cond_wait(&cola->cond, &cola->mutex); // Espera a que se señale que hay un nuevo elemento
    }

    Nodo* nodoDesencolado = cola->cabeza; // Obtiene el nodo al frente de la cola
    void* data = nodoDesencolado->data; // Obtiene los datos del nodo desencolado
    cola->cabeza = nodoDesencolado->next; // Actualiza la cabeza de la cola

    if (cola->cabeza == NULL) { // Si la cola queda vacía después de desencolar
        cola->final = NULL; // Actualiza el final a NULL
    }
    free(nodoDesencolado); // Libera el nodo desencolado
    cola->tam--; // Decrementa el tamaño de la cola
    pthread_mutex_unlock(&cola->mutex); // Desbloquea el mutex

    return data; // Devuelve los datos del nodo desencolado
}

size_t tamCola(Cola* cola) {
    assert(cola != NULL); // Asegura que la cola no sea nula
    pthread_mutex_lock(&cola->mutex); // Bloquea el mutex para leer el tamaño de la cola
    size_t tam = cola->tam; // Obtiene el tamaño actual de la cola
    pthread_mutex_unlock(&cola->mutex); // Desbloquea el mutex
    return tam; // Devuelve el tamaño de la cola
}

