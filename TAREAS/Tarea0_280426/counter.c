#include "counter.h"
#include "pasajero.h"

#include <stdlib.h> // para malloc y free
#include <assert.h> // para assert que detiene el programa si se trata de hacer algo "imposible" como destruir un counter nulo
#include <stdio.h> // para printf
#include <unistd.h> // para sleep
#include <time.h> // para clock_gettime y nanosleep


// funciones internas 
static long calcularDiferenciaMs(struct timespec inicio, struct timespec fin) {
    long segundos = fin.tv_sec - inicio.tv_sec; // Calcula la diferencia en segundos
    long nanosegundos = fin.tv_nsec - inicio.tv_nsec; // Calcula la diferencia en nanosegundos para obtener una mayor precisión
    return segundos * 1000 + nanosegundos / 1000000; // Convierte la diferencia total a milisegundos y la devuelve
}

// elector de k aleatorio en el turno actual dle counter
static int elegirK(Counter* contador) {
    int rango = contador ->kMax - contador->kMin + 1; // Calcula el rango de valores posibles para K
    return contador->kMin + rand_r(&contador->randSeed) % rango; //
}

// Nombre para el tipo de counter 
// para que se vea en el print 
static const char* nombreCounter (TipoCounter tipo) {
    switch (tipo) { // Devuelve el nombre del tipo de counter según su valor
        case TIPO_ECONOMY:
            return "Economy";
        case TIPO_BUSINESS:
            return "Business";
        case TIPO_INTERNATIONAL:
            return "International";
        default:
            return "Desconocido"; // Si el tipo no es reconocido
    }
}

// init  : para inicializar un counter, se le asigna su ID, tipo, cola de la que va a consumir, valores de K, y el flag global de simulacion. Se inicializan las estadisticas y el mutex del counter
void initCounter(Counter* contador, int32_t id, TipoCounter tipo, Cola* cola, int kmin, int kmax, volatile int* activa) { // se usa volatile porque el valor de activa puede cambiar en cualquier momento por otro hilo, y así se le indica al compilador que no optimice el acceso a esa variable asumiendo que no cambia
    assert(contador != NULL); // Asegura que el counter no sea nulo
    assert(cola != NULL); // Asegura que la cola no sea nula
    assert(activa != NULL); // Asegura que el flag de simulacion no sea nulo

    contador ->id = id; // Asigna el ID al counter
    contador->tipo = tipo; // Asigna el tipo al counter
    contador->estado = COUNTER_OPEN; // Inicializa el estado del counter a OPEN
    contador->cola = cola; // Asigna la cola al counter
    contador->kMin = kmin; // Asigna el valor mínimo de K al counter
    contador->kMax = kmax; // Asigna el valor máximo de K al counter
    contador->activa = activa; // Asigna el puntero al flag global de simulacion al counter
    contador->atendidosTurno = 0; // Inicializa el contador de atendidos
    contador->totalAtendidos = 0; // Inicializa el total de atendidos a 0
    contador->sumaEsperaMs = 0; // Inicializa la suma de tiempos de espera a 0
    contador->sumaServicioMs = 0; // Inicializa la suma de tiempos de

    // valor inicial (semilla) única por hilo: se puede usar el ID del counter para generar una semilla diferente para cada hilo, lo que permite que cada counter tenga su propia secuencia de números aleatorios sin interferir con los demás
    contador->randSeed = (unsigned int)(time(NULL)) ^ (unsigned int)(id * 2654435761u); // id se multiplica por un num primo grande para dispersar bits 

    // Se elige primer k del turno 
    contador->kActual = elegirK(contador); 

    pthread_mutex_init(&contador->mutex, NULL); // Inicializa el mutex del counter
    pthread_cond_init(&contador->condReopen, NULL); // Inicializa la variable de condición para reabrir el counter

    printf("Counter %d (%s) inicializado con K en [%d, %d]\n", id, nombreCounter(tipo), kmin, kmax); // Imprime un mensaje indicando que el counter ha sido inicializado
}

void destruirCounter(Counter* c) {
    assert(c != NULL); // Asegura que el counter no sea nulo
    pthread_mutex_destroy(&c->mutex); // Destruye el mutex del counter
    pthread_cond_destroy(&c->condReopen); // Destruye la variable de condición para reabrir el counter
}

// hilo del counter 
void* hilo_counter(void* arg) {
    Counter* contador = (Counter*)arg; // Convierte el argumento a un puntero a Counter
    assert(contador != NULL); // Asegura que el counter no sea nulo

    while(*(contador->activa)) { // Mientras la simulacion activa...

    // Tomar sig pasajero de la cola
    // bloquear bloquea si la cola esta vacia pero para salir si la sim termina se usa tamCola antes de ser bloqueado 

    pthread_mutex_lock(&contador->cola->mutex); // Bloquea el mutex de la cola para acceder a ella

    // Espera cond para despertar a activa cuando no este encendida o despierta 
    while (contador->cola->tam == 0 && *(contador->activa)) { // Mientras la cola esté vacía y la simulación esté activa, espera a que haya un nuevo pasajero o a que la simulación termine
        pthread_cond_wait(&contador->cola->cond, &contador->cola->mutex); // Espera a que se señale que hay un nuevo pasajero en la cola o a que la simulación termine, no se usa busy wait para no consumir CPU innecesariamente
    }

    // si simulacion termina junto con la cola, se sale 
    if (contador->cola->tam == 0 && !*(contador->activa)) { // Si la cola está vacía y la simulación no está activa, sale del hilo
        pthread_mutex_unlock(&contador->cola->mutex); // Desbloquea el mutex de la cola antes de salir
        break; // Sale del bucle principal del hilo
    }

    // PAra extraer a pasajero del frente 
    Nodo* nodoFrente = contador->cola->cabeza; // Obtiene el nodo al frente de la cola
    Pasajero* pasajero = (Pasajero*)nodoFrente->data; // Obtiene el pasajero del nodo al frente de la cola
    
    contador->cola->cabeza = nodoFrente->next; // Actualiza la cabeza de la cola al siguiente nodo
    if (contador->cola->cabeza == NULL) { // Si la cola queda vacía después de desencolar, actualiza el final a NULL
        contador->cola->final = NULL;
    }
    free(nodoFrente); // Libera el nodo desencolado
    contador->cola->tam--; // Se decrementa el tamaño de la cola

    pthread_mutex_unlock(&contador->cola->mutex); // Desbloquea el mutex de la cola después de acceder a ella

    // Inicio de atención al pasajero debe quedar registrado en cuestion de tiempo 
    struct timespec tiempoAtencion;
    clock_gettime(CLOCK_MONOTONIC, &tiempoAtencion);
    pasajero->tiempoAtencion = tiempoAtencion;
    // registrar el counter que atiende al pasajero para las estadisticas y el print
    pasajero->counterId = contador->id; 

     // CAmbiar estado del contador a estar sirviendo activamente 
    pthread_mutex_lock(&contador->mutex); // Bloquea el mutex del counter para cambiar su estado y actualizar estadísticas
    contador->estado = COUNTER_SERVING; // Cambia el estado del counter a SERVING
    pthread_mutex_unlock(&contador->mutex); // Desbloquea el mutex del counter

    // tiempo de servicio 
    usleep((useconds_t)(pasajero->tiempoServicio * 1000)); 
    clock_gettime(CLOCK_MONOTONIC, &pasajero->tiempoFinAtencion); // Registra el tiempo de fin de servicio al pasajero

    long tiempoEsperaMs = calcularDiferenciaMs(pasajero->tiempoLlegada, pasajero->tiempoAtencion); // Calcula el tiempo de espera del pasajero en milisegundos

    printf("Counter %d (%s) atendiendo pasajero %d (%s) | Espera: %ld ms %s\n",
    contador->id,
    nombreCounter(contador->tipo),
    pasajero->id,
    nombre_clase(pasajero->clase),
    tiempoEsperaMs,
    pasajero->redirected ? "| Redirigido" : ""); // Indica si el pasajero fue redirigido o no
    
    //Simulacion de tiempo de servicio 
    long tiempoServicioMs = calcularDiferenciaMs(pasajero->tiempoAtencion, pasajero->tiempoFinAtencion); // Calcula el tiempo de servicio del pasajero en milisegundos

    // Estadisticas
    pthread_mutex_lock(&contador->mutex); // Bloquea el mutex del counter para actualizar estadísticas
    contador->totalAtendidos++; // Incrementa el total de pasajeros atendidos por el counter
    contador->sumaEsperaMs += tiempoEsperaMs; // Suma el tiempo de espera del pasajero al acumulado total de tiempos de espera
    contador->sumaServicioMs += tiempoServicioMs; // Suma el tiempo de servicio del pasajero al acumulado total de tiempos de servicio
    contador->atendidosTurno++; // Incrementa el contador de atendidos en el turno actual
    pthread_mutex_unlock(&contador->mutex); // Desbloquea el mutex del counter después de actualizar estadísticas

    printf("Counter %d (%s) terminó de atender al pasajero %d (%s). Tiempo de servicio: %ld ms\n",
    contador->id,
    nombreCounter(contador->tipo),
    pasajero->id,
    nombre_clase(pasajero->clase),
    tiempoServicioMs);  

    // verificar si debe entrar en break
    pthread_mutex_lock(&contador->mutex); // Bloquea el mutex del counter para verificar si debe entrar en break, se hace con mutex para que si hay otro hiilo leyendo los valores al mismo tiempo, no lo haga 
    int debeEntrarEnBreak = (contador->atendidosTurno >= contador->kActual) && (contador->estado == COUNTER_SERVING); // El counter debe entrar en break si ha atendido al menos K pasajeros en el turno actual y está sirviendo activamente
    pthread_mutex_unlock(&contador->mutex); // Desbloquea el mutex del counter después

    if (debeEntrarEnBreak) {
        pthread_mutex_lock(&contador->mutex); // Bloquea el mutex del counter para cambiar su estado a break
        contador->estado = COUNTER_ON_BREAK; // Cambia el estado del counter a ON_BREAK
        int duracionBreakMs = 500 + (int)(rand_r(&contador->randSeed) % 1501); // Número aleatorio entre 500 y 2000 ms para la duracion que va a tener el break, usa el rand_r de la seed propia del hilo para que no interrumpa a los otros hilos o contadores 
        
        // toma de tiempo actual para calcular exactamente donde es que debe terminar
        clock_gettime(CLOCK_MONOTONIC, &contador->tiempoFinBreak); // Registra el tiempo actual como el inicio del break
        contador->tiempoFinBreak.tv_sec += duracionBreakMs / 1000;
        contador->tiempoFinBreak.tv_nsec += (duracionBreakMs % 1000) * 1000000; // Suma la duración del break al tiempo de fin del break, se hace en dos partes para convertir correctamente a segundos y nanosegundos

        if (contador->tiempoFinBreak.tv_nsec >= 1000000000) { // Si los nanosegundos exceden 1 segundo, ajusta el tiempo de fin del break
            contador->tiempoFinBreak.tv_sec += contador->tiempoFinBreak.tv_nsec / 1000000000; // Suma los segundos adicionales al tiempo de fin del break
            contador->tiempoFinBreak.tv_nsec = contador->tiempoFinBreak.tv_nsec % 1000000000; // Ajusta los nanosegundos para que queden en el rango correcto
        } 

        printf("Counter %d (%s) entrando en break por %d ms. Atendió %d pasajeros)\n",
            contador->id, //%d
            nombreCounter(contador->tipo), //%s
            duracionBreakMs, //%d
            contador->atendidosTurno); // Imprime un mensaje indicando que el counter ha entr
        pthread_cond_wait(&contador->condReopen, &contador->mutex); // Counter duerme aqui esperando a que supervisor lo despierte, al esperar suelta el mutex para ue sup lo tome. se hace para que no haya deadlock ya que los dos neecsitan el mutex
        // Al despertar, el counter se reabre y se elige un nuevo K para el siguiente turno
        contador->estado = COUNTER_OPEN; // Cambia el estado del counter a OPEN al reabrir
        contador->atendidosTurno = 0; // Reinicia el contador de atendidos en el turno al reabrir
        contador->kActual = elegirK(contador); 
        printf("Counter %d (%s) reabierto por Supervisor. Nuevo K para el siguiente turno: %d\n", contador->id, nombreCounter(contador->tipo), contador->kActual);
        pthread_mutex_unlock(&contador->mutex); // Desbloquea el mutex del counter después de reabrir
        }
    } // fin del while de la simulacion activa
    return NULL;
} // se cierra hilo de counter 
