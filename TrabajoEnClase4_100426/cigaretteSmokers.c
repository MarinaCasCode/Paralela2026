// Ejercicio Cigarette Smokers
// Jimena Bejarano Sanchez C31074
// Marina Castro Peralta C31886
// gcc cigaretteSmokers.c -o cigaretteSmokers -lpthread

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

pthread_t fumadorFosforo, fumadorPapel, fumadorTabaco, agente;
pthread_mutex_t mutex     = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t condFosforo = PTHREAD_COND_INITIALIZER;
pthread_cond_t condPapel   = PTHREAD_COND_INITIALIZER;
pthread_cond_t condTabaco  = PTHREAD_COND_INITIALIZER;

// Variables para representar los ingredientes disponibles
int ingredienteFosforo = 0;
int ingredientePapel   = 0;
int ingredienteTabaco  = 0;

// Función que verifica qué fumador debe ser despertado según los ingredientes disponibles
void despertador(void) {
    if (ingredienteFosforo && ingredientePapel) {
        ingredienteFosforo = 0;
        ingredientePapel   = 0;
        printf("[Despertador] Hay fosforo y papel: avisa al fumador de tabaco\n");
        pthread_cond_signal(&condTabaco);
    } else if (ingredienteFosforo && ingredienteTabaco) {
        ingredienteFosforo = 0;
        ingredienteTabaco  = 0;
        printf("[Despertador] Hay fosforo y tabaco: avisa al fumador de papel\n");
        pthread_cond_signal(&condPapel);
    } else if (ingredientePapel && ingredienteTabaco) {
        ingredientePapel  = 0;
        ingredienteTabaco = 0;
        printf("[Despertador] Hay papel y tabaco: avisa al fumador de fosforo\n");
        pthread_cond_signal(&condFosforo);
    }
}

// Hilo del agente que coloca dos ingredientes al azar
void* agenteFunc(void* arg) {
    (void)arg;
    while (1) {
        int combo = rand() % 3;
        pthread_mutex_lock(&mutex);
        switch (combo) {
            case 0:
                ingredienteFosforo = 1;
                ingredientePapel   = 1;
                printf("[Agente] Pone fosforo y papel\n");
                break;
            case 1:
                ingredienteFosforo = 1;
                ingredienteTabaco  = 1;
                printf("[Agente] Pone fosforo y tabaco\n");
                break;
            case 2:
                ingredientePapel  = 1;
                ingredienteTabaco = 1;
                printf("[Agente] Pone papel y tabaco\n");
                break;
        }
        despertador(); // Verifica qué fumador debe ser despertado
        pthread_mutex_unlock(&mutex);
        sleep(2);
    }
    return NULL;
}

// Hilo del fumador que tiene fósforo
void* fumadorFosforoFunc(void* arg) {
    (void)arg;
    while (1) {
        pthread_mutex_lock(&mutex);
        pthread_cond_wait(&condFosforo, &mutex);
        printf("[Fumador fosforo] Fumando...\n");
        pthread_mutex_unlock(&mutex);
        sleep(1);
    }
    return NULL;
}

// Hilo del fumador que tiene papel
void* fumadorPapelFunc(void* arg) {
    (void)arg;
    while (1) {
        pthread_mutex_lock(&mutex);
        pthread_cond_wait(&condPapel, &mutex);
        printf("[Fumador papel] Fumando...\n");
        pthread_mutex_unlock(&mutex);
        sleep(1);
    }
    return NULL;
}

// Hilo del fumador que tiene tabaco
void* fumadorTabacoFunc(void* arg) {
    (void)arg;
    while (1) {
        pthread_mutex_lock(&mutex);
        pthread_cond_wait(&condTabaco, &mutex);
        printf("[Fumador tabaco] Fumando...\n");
        pthread_mutex_unlock(&mutex);
        sleep(1);
    }
    return NULL;
}

int main(void) {
    srand(time(NULL));
    
    // Crear los hilos para el agente y los fumadores
    pthread_create(&fumadorFosforo, NULL, fumadorFosforoFunc, NULL);
    pthread_create(&fumadorPapel,   NULL, fumadorPapelFunc,   NULL);
    pthread_create(&fumadorTabaco,  NULL, fumadorTabacoFunc,  NULL);
    pthread_create(&agente,         NULL, agenteFunc,         NULL);

    // Esperar a que los hilos terminen
    pthread_join(agente,         NULL);
    pthread_join(fumadorFosforo, NULL);
    pthread_join(fumadorPapel,   NULL);
    pthread_join(fumadorTabaco,  NULL);
    return 0;
}