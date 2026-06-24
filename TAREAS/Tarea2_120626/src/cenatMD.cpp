// cenatMD.cpp

// Compilacion: ver Makefile (mpicxx -O2 -fopenmp)
// Ejecucion:   mpiexec -np <#Ranks> ./cenatMD <N> <ITERACIONES> <BANDERA_IMP> <BANDERA_INI>

#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <mpi.h>
#include <omp.h>

// ----- Constantes fisicas del problema (dadas en el enunciado) -----
static const double A_COEF  = 2.0;
static const double B_COEF  = 1.0;
static const double MASS    = 4.0;
static const double DELTA_T = 0.1;

// ----- Constantes de la inicializacion -----
static const int    LATTICE_SIDE = 10;
static const double LATTICE_GAP  = 1.2;
static const double RANDOM_BOX   = 12.0;

// ===================================================================
// Modelo de particulas en SoA (un bloque contiguo de 9*n doubles).
// ===================================================================
struct ParticleSet {
    int n;
    double* x;  double* y;  double* z;
    double* vx; double* vy; double* vz;
    double* fx; double* fy; double* fz;
};

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

static void free_particles(ParticleSet* p) {
    free(p->x);
    p->x = p->y = p->z = NULL;
    p->vx = p->vy = p->vz = NULL;
    p->fx = p->fy = p->fz = NULL;
    p->n = 0;
}

static inline double next_random(unsigned int* state) {
    *state = (*state) * 1103515245u + 12345u;
    return (double) ((*state >> 16) & 0x7FFF) / 32767.0;
}

static void initialize(ParticleSet* p, int n, int fixed, int global_offset, unsigned int seed) {
    for (int i = 0; i < n; i++) {
        if (fixed) {
            int gid = global_offset + i;
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
// evolve: fuerzas de Van der Waals entre dos conjuntos (3ra ley de Newton)
// paralelizado con OpenMP
// ===================================================================
static void evolve(ParticleSet* A, ParticleSet* B, int nA, int nB) {
    if (A == B) {
        // Auto-interaccion (fuerzas internas). Cada par (i,j) con j>i una vez.
        const int n = nA;
        double* fx = A->fx; double* fy = A->fy; double* fz = A->fz;
        #pragma omp parallel for schedule(guided) \
            reduction(+:fx[0:n], fy[0:n], fz[0:n])
        for (int i = 0; i < n; i++) {
            for (int j = i + 1; j < n; j++) {
                double rx = A->x[i] - A->x[j];
                double ry = A->y[i] - A->y[j];
                double rz = A->z[i] - A->z[j];
                double r2 = rx*rx + ry*ry + rz*rz;
                if (r2 == 0.0) continue;
                double inv_r   = 1.0 / sqrt(r2);
                double inv_r2  = inv_r * inv_r;
                double inv_r6  = inv_r2 * inv_r2 * inv_r2;
                double inv_r12 = inv_r6 * inv_r6;
                double f = B_COEF * inv_r12 - A_COEF * inv_r6;
                double cfx = f * rx * inv_r, cfy = f * ry * inv_r, cfz = f * rz * inv_r;
                fx[i] += cfx; fy[i] += cfy; fz[i] += cfz;
                fx[j] -= cfx; fy[j] -= cfy; fz[j] -= cfz;
            }
        }
    } else {
        const int nb = nB;
        double* Bfx = B->fx; double* Bfy = B->fy; double* Bfz = B->fz;
        #pragma omp parallel for schedule(static) \
            reduction(+:Bfx[0:nb], Bfy[0:nb], Bfz[0:nb])
        for (int i = 0; i < nA; i++) {
            double afx = 0.0, afy = 0.0, afz = 0.0;   // fuerza local acumulada (sin carrera)
            for (int j = 0; j < nB; j++) {
                double rx = A->x[i] - B->x[j];
                double ry = A->y[i] - B->y[j];
                double rz = A->z[i] - B->z[j];
                double r2 = rx*rx + ry*ry + rz*rz;
                if (r2 == 0.0) continue;
                double inv_r   = 1.0 / sqrt(r2);
                double inv_r2  = inv_r * inv_r;
                double inv_r6  = inv_r2 * inv_r2 * inv_r2;
                double inv_r12 = inv_r6 * inv_r6;
                double f = B_COEF * inv_r12 - A_COEF * inv_r6;
                double cfx = f * rx * inv_r, cfy = f * ry * inv_r, cfz = f * rz * inv_r;
                afx += cfx; afy += cfy; afz += cfz;
                Bfx[j] -= cfx; Bfy[j] -= cfy; Bfz[j] -= cfz;
            }
            A->fx[i] += afx; A->fy[i] += afy; A->fz[i] += afz;
        }
    }
}

// merge: suma a las locales las reacciones que las remotas trajeron de regreso.
static void merge(ParticleSet* locals, ParticleSet* returned, int n) {
    for (int i = 0; i < n; i++) {
        locals->fx[i] += returned->fx[i];
        locals->fy[i] += returned->fy[i];
        locals->fz[i] += returned->fz[i];
    }
}

// updateProperties: aceleracion -> velocidad -> posicion (Euler semi-implicito)
static void updateProperties(ParticleSet* p, int n) {
    #pragma omp parallel for schedule(static)
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
        p->fx[i] = 0.0; p->fy[i] = 0.0; p->fz[i] = 0.0;
    }
}

// ===================================================================
// Empaque/desempaque de posicion+fuerza (6 columnas) en un buffer
// contiguo de 6*n doubles, para enviarlo en UN solo mensaje MPI.
// ===================================================================
static void pack6(const ParticleSet* p, int n, double* buf) {
    for (int i = 0; i < n; i++) {
        buf[0*n+i] = p->x[i];  buf[1*n+i] = p->y[i];  buf[2*n+i] = p->z[i];
        buf[3*n+i] = p->fx[i]; buf[4*n+i] = p->fy[i]; buf[5*n+i] = p->fz[i];
    }
}
static void unpack6(ParticleSet* p, int n, const double* buf) {
    for (int i = 0; i < n; i++) {
        p->x[i]  = buf[0*n+i]; p->y[i]  = buf[1*n+i]; p->z[i]  = buf[2*n+i];
        p->fx[i] = buf[3*n+i]; p->fy[i] = buf[4*n+i]; p->fz[i] = buf[5*n+i];
    }
}

// ===================================================================
// compute_forces_ring: fuerza total sobre las locales usando el anillo (a-f).
//   nsteps = size/2  ->  (size-1)/2 si size impar, size/2 si size par.
// ===================================================================
static void compute_forces_ring(ParticleSet* locals, ParticleSet* remotes,
                                double* sendbuf, double* recvbuf,
                                int n, int rank, int size) {
    // remotes = copia de las posiciones locales, con fuerzas en cero.
    for (int i = 0; i < n; i++) {
        remotes->x[i] = locals->x[i];
        remotes->y[i] = locals->y[i];
        remotes->z[i] = locals->z[i];
        remotes->fx[i] = 0.0; remotes->fy[i] = 0.0; remotes->fz[i] = 0.0;
    }

    int nsteps = size / 2;
    int right  = (rank + 1) % size;
    int left   = (rank - 1 + size) % size;
    MPI_Status st;

    // Medio-anillo: las remotas se desplazan a la derecha; cada rank calcula
    // la interaccion con las remotas que lo visitan (pasos a-d).
    for (int step = 1; step <= nsteps; step++) {
        pack6(remotes, n, sendbuf);
        MPI_Sendrecv(sendbuf, 6*n, MPI_DOUBLE, right, 0,
                     recvbuf, 6*n, MPI_DOUBLE, left,  0,
                     MPI_COMM_WORLD, &st);
        unpack6(remotes, n, recvbuf);

        bool do_evolve = (size % 2 == 1) || (step < nsteps) || (rank < size / 2);
        if (do_evolve) {
            evolve(locals, remotes, n, n);
        }
    }

    // Paso e: devolver las remotas a su procesador original y combinar.
    int dest = (rank - nsteps + size) % size;   // origen de las remotas que tengo
    int src  = (rank + nsteps) % size;           // quien me devuelve MIS remotas
    pack6(remotes, n, sendbuf);
    MPI_Sendrecv(sendbuf, 6*n, MPI_DOUBLE, dest, 1,
                 recvbuf, 6*n, MPI_DOUBLE, src,  1,
                 MPI_COMM_WORLD, &st);
    unpack6(remotes, n, recvbuf);   // remotes = MIS particulas con las reacciones recogidas

    merge(locals, remotes, n);      // paso f (parte 1): sumar reacciones de regreso
    evolve(locals, locals, n, n);   // paso f (parte 2): fuerzas internas propias
}

// ===================================================================
// gather_and_write: rank 0 recolecta (MPI_Gather) las posiciones de todas
// las particulas y escribe un CSV en orden global.
// ===================================================================
static void gather_and_write(ParticleSet* locals, int n, int rank, int size, int iter,
                             double* gsend, double* grecv) {
    for (int i = 0; i < n; i++) {
        double vmag = sqrt(locals->vx[i]*locals->vx[i] +
                           locals->vy[i]*locals->vy[i] +
                           locals->vz[i]*locals->vz[i]);
        gsend[4*i+0] = locals->x[i];
        gsend[4*i+1] = locals->y[i];
        gsend[4*i+2] = locals->z[i];
        gsend[4*i+3] = vmag;
    }

    MPI_Gather(gsend, 4*n, MPI_DOUBLE, grecv, 4*n, MPI_DOUBLE, 0, MPI_COMM_WORLD);

    if (rank == 0) {
        char filename[64];
        snprintf(filename, sizeof(filename), "posiciones_%06d.csv", iter);
        FILE* file = fopen(filename, "w");
        if (file == NULL) {
            fprintf(stderr, "Error: no se pudo abrir %s\n", filename);
            return;
        }
        fprintf(file, "x,y,z,vmag\n");
        int total = n * size;
        for (int g = 0; g < total; g++) {
            fprintf(file, "%f,%f,%f,%f\n",
                    grecv[4*g+0], grecv[4*g+1], grecv[4*g+2], grecv[4*g+3]);
        }
        fclose(file);
    }
}

int main(int argc, char* argv[]) {
    MPI_Init(&argc, &argv);
    int rank = 0, size = 0;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    int N       = (argc >= 2) ? atoi(argv[1]) : 100;
    int iters   = (argc >= 3) ? atoi(argv[2]) : 100;
    int flagImp = (argc >= 4) ? atoi(argv[3]) : 0;
    int flagIni = (argc >= 5) ? atoi(argv[4]) : 0;

    if (N < 1 || iters < 1) {
        if (rank == 0)
            fprintf(stderr, "Uso: mpiexec -np <R> ./cenatMD <N> <ITER> <IMP> <INI>\n");
        MPI_Finalize();
        return EXIT_FAILURE;
    }

    if (rank == 0) {
        printf("=== cenatMD (Fase 4: MPI + OpenMP) ===\n");
        printf("Procesos MPI = %d | hilos OpenMP/rank = %d | N/proc = %d | total = %d | iter = %d\n",
               size, omp_get_max_threads(), N, N * size, iters);
        printf("BANDERA_IMP = %d | BANDERA_INI = %d\n", flagImp, flagIni);
    }

    ParticleSet locals, remotes;
    allocate_particles(&locals, N);
    allocate_particles(&remotes, N);
    double* sendbuf = (double*) malloc((size_t) 6 * N * sizeof(double));
    double* recvbuf = (double*) malloc((size_t) 6 * N * sizeof(double));
    double* gsend   = (double*) malloc((size_t) 4 * N * sizeof(double));
    double* grecv   = (double*) malloc((size_t) 4 * N * size * sizeof(double));

    initialize(&locals, N, flagIni, rank * N, (unsigned int)(rank + 1));

    double t0 = MPI_Wtime();
    for (int it = 0; it < iters; it++) {
        compute_forces_ring(&locals, &remotes, sendbuf, recvbuf, N, rank, size);
        updateProperties(&locals, N);
        if (flagImp && (it % 100 == 0)) {
            gather_and_write(&locals, N, rank, size, it, gsend, grecv);
        }
    }
    double t1 = MPI_Wtime();
    if (rank == 0)
        printf("Simulacion terminada en %.4f segundos\n", t1 - t0);

    free(sendbuf); free(recvbuf); free(gsend); free(grecv);
    free_particles(&locals);
    free_particles(&remotes);
    MPI_Finalize();
    return 0;
}