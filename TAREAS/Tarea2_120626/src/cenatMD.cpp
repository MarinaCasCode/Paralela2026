// cenatMD.cpp

// Compilacion: ver Makefile (mpicxx -O2 -fopenmp)
// Ejecucion:   mpiexec -np <#Ranks> ./cenatMD <N> <ITERACIONES> <BANDERA_IMP> <BANDERA_INI>
//   N            - cantidad de particulas POR proceso
//   ITERACIONES  - cantidad de iteraciones de la simulacion
//   BANDERA_IMP  - 1 = escribir archivo de posiciones cada 100 iteraciones
//   BANDERA_INI  - 1 = inicializar particulas en posiciones fijas (validacion)

#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <mpi.h>

// ----- Constantes fisicas del problema (dadas en el enunciado) -----
static const double A_COEF  = 2.0;   // coeficiente del termino atractivo (r^6)
static const double B_COEF  = 1.0;   // coeficiente del termino repulsivo (r^12)
static const double MASS    = 4.0;   // masa de cada particula
static const double DELTA_T = 0.1;   // paso de tiempo de la integracion

// ----- Constantes de la inicializacion -----
static const int    LATTICE_SIDE = 10;    // particulas por lado en la malla (init fija)
static const double LATTICE_GAP  = 1.2;   // separacion entre particulas en la malla
static const double RANDOM_BOX   = 12.0;  // tamano de la caja para init aleatoria

// ===================================================================
// Modelo de particulas en SoA (Structure of Arrays)
// Cada propiedad es un arreglo contiguo: favorece la vectorizacion (SIMD)
// del kernel de fuerzas en fases posteriores. Las 9 columnas viven en un
// unico bloque de memoria para tener buena localidad y liberar con un free().
// ===================================================================
struct ParticleSet {
    int n;                                 // cantidad de particulas
    double* x;  double* y;  double* z;     // posiciones
    double* vx; double* vy; double* vz;    // velocidades
    double* fx; double* fy; double* fz;    // fuerzas acumuladas en la iteracion
};

// Reserva memoria para n particulas (un solo bloque de 9*n doubles).
static void allocate_particles(ParticleSet* p, int n) {
    p->n = n;
    double* block = (double*) malloc((size_t) 9 * n * sizeof(double));
    if (block == NULL) {
        fprintf(stderr, "Error: no se pudo asignar memoria para %d particulas\n", n);
        MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
    }
    p->x  = block + 0 * n;  p->y  = block + 1 * n;  p->z  = block + 2 * n;
    p->vx = block + 3 * n;  p->vy = block + 4 * n;  p->vz = block + 5 * n;
    p->fx = block + 6 * n;  p->fy = block + 7 * n;  p->fz = block + 8 * n;
}

// Libera la memoria reservada por allocate_particles.
static void free_particles(ParticleSet* p) {
    free(p->x);   // x apunta al inicio del bloque contiguo
    p->x = p->y = p->z = NULL;
    p->vx = p->vy = p->vz = NULL;
    p->fx = p->fy = p->fz = NULL;
    p->n = 0;
}

// Generador congruencial lineal simple y reentrante (estado por parametro).
static inline double next_random(unsigned int* state) {
    *state = (*state) * 1103515245u + 12345u;
    return (double) ((*state >> 16) & 0x7FFF) / 32767.0;   // valor en [0,1]
}

// ===================================================================
// Inicializacion de particulas
//   fixed = 1 -> posiciones deterministas en una malla 3D (para validacion)
//   fixed = 0 -> posiciones aleatorias dentro de una caja
//   global_offset: indice global de la primera particula de este proceso.
//   seed: semilla del generador (distinta por proceso).
// Velocidades y fuerzas inician en cero en ambos casos.
// ===================================================================
static void initialize(ParticleSet* p, int n, int fixed, int global_offset, unsigned int seed) {
    for (int i = 0; i < n; i++) {
        if (fixed) {
            int gid = global_offset + i;                       // indice global
            int ix = gid % LATTICE_SIDE;
            int iy = (gid / LATTICE_SIDE) % LATTICE_SIDE;
            int iz = gid / (LATTICE_SIDE * LATTICE_SIDE);
            p->x[i] = ix * LATTICE_GAP;
            p->y[i] = iy * LATTICE_GAP;
            p->z[i] = iz * LATTICE_GAP;
        } else {
            p->x[i] = next_random(&seed) * RANDOM_BOX;
            p->y[i] = next_random(&seed) * RANDOM_BOX;
            p->z[i] = next_random(&seed) * RANDOM_BOX;
        }
        p->vx[i] = 0.0; p->vy[i] = 0.0; p->vz[i] = 0.0;
        p->fx[i] = 0.0; p->fy[i] = 0.0; p->fz[i] = 0.0;
    }
}

// ===================================================================
// evolve: calcula las fuerzas de Van der Waals entre dos conjuntos y las
// acumula (3ra ley de Newton: la fuerza sobre A es igual y opuesta a la de B).
//   - A y B distintos (anillo: locales vs remotas): se evaluan todos los
//     pares (i de A, j de B).
//   - A y B el mismo conjunto (fuerzas propias, paso f): cada par una sola
//     vez (j > i) y se omite la auto-fuerza (i == j).
// ===================================================================
static void evolve(ParticleSet* A, ParticleSet* B, int nA, int nB) {
    const bool self = (A == B);   // mismo conjunto -> auto-interaccion

    for (int i = 0; i < nA; i++) {
        int j0 = self ? (i + 1) : 0;   // en auto-interaccion contamos cada par una vez
        for (int j = j0; j < nB; j++) {
            double rx = A->x[i] - B->x[j];
            double ry = A->y[i] - B->y[j];
            double rz = A->z[i] - B->z[j];
            double r2 = rx * rx + ry * ry + rz * rz;
            if (r2 == 0.0) continue;   // evita division por cero (particulas coincidentes)

            double inv_r   = 1.0 / sqrt(r2);            // 1/r
            double inv_r2  = inv_r * inv_r;             // 1/r^2
            double inv_r6  = inv_r2 * inv_r2 * inv_r2;  // 1/r^6
            double inv_r12 = inv_r6 * inv_r6;           // 1/r^12

            double f = B_COEF * inv_r12 - A_COEF * inv_r6;   // magnitud: B/r^12 - A/r^6

            double fx = f * rx * inv_r;   // componente = f * (r_comp / r)
            double fy = f * ry * inv_r;
            double fz = f * rz * inv_r;

            A->fx[i] += fx;  A->fy[i] += fy;  A->fz[i] += fz;
            B->fx[j] -= fx;  B->fy[j] -= fy;  B->fz[j] -= fz;
        }
    }
}

// ===================================================================
// merge: suma a las fuerzas locales las reacciones que las particulas
// "remotas" acumularon mientras viajaban por el anillo y regresaron al
// origen. Ambos conjuntos tienen las mismas n particulas en el mismo orden.
// ===================================================================
static void merge(ParticleSet* locals, ParticleSet* returned, int n) {
    for (int i = 0; i < n; i++) {
        locals->fx[i] += returned->fx[i];
        locals->fy[i] += returned->fy[i];
        locals->fz[i] += returned->fz[i];
    }
}

// ===================================================================
// updateProperties: con la fuerza total acumulada calcula aceleracion,
// velocidad y posicion nuevas (Euler semi-implicito: primero velocidad,
// luego posicion con la velocidad ya actualizada). Al final reinicia las
// fuerzas en cero para la siguiente iteracion.
// ===================================================================
static void updateProperties(ParticleSet* p, int n) {
    for (int i = 0; i < n; i++) {
        double ax = p->fx[i] / MASS;
        double ay = p->fy[i] / MASS;
        double az = p->fz[i] / MASS;

        p->vx[i] += ax * DELTA_T;
        p->vy[i] += ay * DELTA_T;
        p->vz[i] += az * DELTA_T;

        p->x[i] += p->vx[i] * DELTA_T;
        p->y[i] += p->vy[i] * DELTA_T;
        p->z[i] += p->vz[i] * DELTA_T;

        p->fx[i] = 0.0; p->fy[i] = 0.0; p->fz[i] = 0.0;   // reset para la proxima iteracion
    }
}

// ===================================================================
// write_positions: escribe posiciones (y rapidez, para colorear en Paraview)
// de todas las particulas a un CSV. Un archivo por punto de control.
// ===================================================================
static void write_positions(ParticleSet* p, int n, int iter) {
    char filename[64];
    snprintf(filename, sizeof(filename), "posiciones_%06d.csv", iter);
    FILE* file = fopen(filename, "w");
    if (file == NULL) {
        fprintf(stderr, "Error: no se pudo abrir %s para escribir\n", filename);
        return;
    }
    fprintf(file, "x,y,z,vmag\n");
    for (int i = 0; i < n; i++) {
        double vmag = sqrt(p->vx[i]*p->vx[i] + p->vy[i]*p->vy[i] + p->vz[i]*p->vz[i]);
        fprintf(file, "%f,%f,%f,%f\n", p->x[i], p->y[i], p->z[i], vmag);
    }
    fclose(file);
}

int main(int argc, char* argv[]) {
    MPI_Init(&argc, &argv);

    int rank = 0, size = 0;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    // ----- Argumentos: N ITERACIONES BANDERA_IMP BANDERA_INI -----
    int N       = (argc >= 2) ? atoi(argv[1]) : 100;   // particulas por proceso
    int iters   = (argc >= 3) ? atoi(argv[2]) : 100;   // iteraciones
    int flagImp = (argc >= 4) ? atoi(argv[3]) : 0;     // imprimir cada 100 iter
    int flagIni = (argc >= 5) ? atoi(argv[4]) : 0;     // posiciones fijas

    if (N < 1 || iters < 1) {
        if (rank == 0)
            fprintf(stderr, "Uso: mpiexec -np <R> ./cenatMD <N> <ITER> <IMP> <INI>\n");
        MPI_Finalize();
        return EXIT_FAILURE;
    }

    if (rank == 0) {
        printf("=== cenatMD (Fase 1: simulacion serial de referencia) ===\n");
        printf("Procesos MPI = %d | N por proceso = %d | iteraciones = %d\n", size, N, iters);
        printf("BANDERA_IMP = %d | BANDERA_INI = %d\n", flagImp, flagIni);
        if (size > 1) {
            printf("AVISO: la Fase 1 solo resuelve el caso de 1 proceso.\n");
            printf("       El anillo MPI para size>1 llega en la Fase 2.\n");
        }
    }

    // ----- Inicializacion -----
    ParticleSet locals;
    allocate_particles(&locals, N);
    initialize(&locals, N, flagIni, rank * N, (unsigned int)(rank + 1));

    // ----- Bucle principal -----
    double t_start = MPI_Wtime();
    for (int it = 0; it < iters; it++) {
        // -------------------------------------------------------------
        //  aqui ira el anillo MPI cuando size > 1:
        //   a-d) medio-anillo: evolve(locals, remotes) por (p-1)/2 pasos
        //   e)   devolver remotas al origen
        //   f)   merge(locals, returned) para sumar las fuerzas que regresaron
        // -------------------------------------------------------------
        evolve(&locals, &locals, N, N);   // paso f: fuerzas propias
        updateProperties(&locals, N);     // paso g: nuevas posiciones

        if (flagImp && (it % 100 == 0)) {
            write_positions(&locals, N, it);
        }
    }
    double t_end = MPI_Wtime();

    if (rank == 0)
        printf("Simulacion terminada en %.4f segundos\n", t_end - t_start);

    free_particles(&locals);
    MPI_Finalize();
    return 0;
}