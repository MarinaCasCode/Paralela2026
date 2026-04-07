// Ejercicio Dining Philosophers
// Jimena Bejarano Sanchez C31074
// Marina Castro Peralta C31886
// gcc diningPhilosophers.c -o diningPhilosophers -lpthread

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

// Hace que sea compatible con windows y linux 
#ifdef _WIN32
#include <windows.h>
#define dormir(segundos) Sleep((segundos) * 1000)
#else
#include <unistd.h>
#define dormir(segundos) sleep(segundos)
#endif

// Duración que va a tomar el turno y estar comiendo para cada filósofo 
#define DURACION_TURNO 3
#define DURACION_COMIDA 1

// Variables globales 
// Tenedores son hilos que se encargan de controlar los turnos , filosofo comiendo o filosofo pensando, cuando no hya tenedores 
pthread_mutex_t* tenedores; // tenedores
pthread_mutex_t mutexTurno = PTHREAD_MUTEX_INITIALIZER;

int cantidadFilosofos;
int comenImpares = 1; // 1: comen impares, 0: comen pares

// Función para obtener la cantidad de núcleos del sistema
// SI no se puede obtener la cantidad de núcleos, se sume un valor de estos
int obtenerCantidadNucleos(void) {
#ifdef _WIN32
    SYSTEM_INFO info;
    GetSystemInfo(&info);
    return (int)info.dwNumberOfProcessors;
#else
    int nucleos = 20;
    FILE* pipe = popen("nproc", "r");

    if (pipe != NULL) {
        if (fscanf(pipe, "%d", &nucleos) != 1 || nucleos < 2) {
            nucleos = 20;
        }
        pclose(pipe);
    }

    return nucleos;
#endif
}

// Hilo que alterna los turnos entre filósofos impares y pares cada DURACION_TURNO segundos
void* alternarTurnos(void* arg) {
    (void)arg;

    // mientras sea verdadero, va alternando los turnos
    while (1) {
        dormir(DURACION_TURNO);
        // alterna el turno entre impares y pares
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

    // ciclo infinito donde el filosofo piensa o come dependiendo del turno
    while (1) {
        pthread_mutex_lock(&mutexTurno);
        puedeComer = (comenImpares == esImpar);
        pthread_mutex_unlock(&mutexTurno);

        if (!puedeComer) {
            printf("Filosofo %d piensa\n", id + 1);
            dormir(1);
            continue;
        }

        // toma ambos tenedores para poder comer
        pthread_mutex_lock(&tenedores[id]); // toma el tenedor izquierdo
        pthread_mutex_lock(&tenedores[(id + 1) % cantidadFilosofos]); // toma el tenedor derecho
        printf("Filosofo %d come\n", id + 1);
        dormir(DURACION_COMIDA);
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
    if (cantidadFilosofos % 2 != 0) { // asumimos que la cantidad de nucleos suele ser par
        cantidadFilosofos--;
    }
    if (cantidadFilosofos < 2) {
        cantidadFilosofos = 2;
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

