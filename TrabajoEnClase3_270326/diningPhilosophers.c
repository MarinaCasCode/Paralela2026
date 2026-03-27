// Ejercicio Dining Philosophers
// Jimena Bejarano Sanchez C31074
// Marina Castro Peralta C31886
// gcc diningPhilosophers.c -o diningPhilosophers -lpthread

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

#define DURACION_TURNO 3
#define DURACION_COMIDA 1

pthread_mutex_t* tenedores; // tenedores
pthread_mutex_t mutexTurno = PTHREAD_MUTEX_INITIALIZER;

int cantidadFilosofos;
int comenImpares = 1; // 1: comen impares, 0: comen pares

int obtenerCantidadNucleos(void) {
    int nucleos;
    FILE* pipe = popen("nproc", "r");

    fscanf(pipe, "%d", &nucleos);
    pclose(pipe);
    return nucleos;
}

void* alternarTurnos(void* arg) {
    (void)arg;

    while (1) {
        sleep(DURACION_TURNO);

        pthread_mutex_lock(&mutexTurno);
        comenImpares = 1 - comenImpares;
        printf("Turno: %s\n", comenImpares ? "impares" : "pares");
        pthread_mutex_unlock(&mutexTurno);
    }

    return NULL;
}

void* cicloFilosofo(void* arg) {
    int id = *(int*)arg; // identificador del filosofo
    int esImpar = id % 2;
    int puedeComer;

    while (1) {
        pthread_mutex_lock(&mutexTurno);
        puedeComer = (comenImpares == esImpar);
        pthread_mutex_unlock(&mutexTurno);

        if (!puedeComer) {
            printf("F%d piensa\n", id);
            sleep(1);
            continue;
        }

        // toma ambos tenedores para poder comer
        pthread_mutex_lock(&tenedores[id]); // toma el tenedor izquierdo
        pthread_mutex_lock(&tenedores[(id + 1) % cantidadFilosofos]); // toma el tenedor derecho
        printf("F%d come\n", id);
        sleep(DURACION_COMIDA);
        // suelta ambos tenedores al terminar
        pthread_mutex_unlock(&tenedores[id]); // suelta el tenedor izquierdo
        pthread_mutex_unlock(&tenedores[(id + 1) % cantidadFilosofos]); // suelta el tenedor derecho
    }
    return NULL;
}

int main() {
    pthread_t hiloControlador;
    pthread_t* hilosFilosofos;
    int* identificadores;

    cantidadFilosofos = obtenerCantidadNucleos();
    if (cantidadFilosofos % 2 != 0) {
        cantidadFilosofos--;
    }

    tenedores = malloc(sizeof(pthread_mutex_t) * cantidadFilosofos);
    hilosFilosofos = malloc(sizeof(pthread_t) * cantidadFilosofos);
    identificadores = malloc(sizeof(int) * cantidadFilosofos);

    // inicializa los tenedores y los identificadores
    for (int i = 0; i < cantidadFilosofos; i++) {
        pthread_mutex_init(&tenedores[i], NULL);
        identificadores[i] = i; // asigna id a cada filosofo
    }

    pthread_create(&hiloControlador, NULL, alternarTurnos, NULL);

    // crea los hilos que ejecutan el ciclo de cada filosofo
    for (int i = 0; i < cantidadFilosofos; i++) {
        pthread_create(&hilosFilosofos[i], NULL, cicloFilosofo, &identificadores[i]);
    }

    pthread_join(hiloControlador, NULL);

    // espera a que los hilos terminen
    for (int i = 0; i < cantidadFilosofos; i++) {
        pthread_join(hilosFilosofos[i], NULL);
    }

    return 0;
}

