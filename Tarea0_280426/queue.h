#ifndef QUEUE_H
#define QUEUE_H

#include <pthread.h>
#include <stddef.h> // para size_t que es mejor que int para tamaños

// Lista enlazada para la cola 
typedef struct Nodo {
    void* data; // Puntero a los datos almacenados
    struct Nodo* next; // Puntero al siguiente nodo
} Nodo; // crea Nodo para escribir solo Nodo en vez de struct Nodo

// Cola 
typedef struct Cola {
    Nodo* cabeza; // Puntero al frente de la cola
    Nodo* final;  // Puntero al final de la cola
    size_t tam; // Tamaño actual de la cola
    pthread_mutex_t mutex; // Mutex para sincronización
    pthread_cond_t cond;   // Variable de condición para esperar/avisar
} Cola;

// Funciones para manejar la cola
void initCola(Cola* cola); // Inicializa la cola
void destruirCola(Cola* cola); // Destruye la cola y libera 
void encolar(Cola* cola, void* data); // Agrega un elemento a la cola
void* desencolar(Cola* cola); // Elimina y devuelve el elemento al frente de la cola, bloquea si la cola está vacía
size_t tamCola(Cola* cola); // Devuelve el tamaño actual de la cola

#endif // QUEUE_H