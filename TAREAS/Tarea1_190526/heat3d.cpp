// heat3d.cpp
// Tarea 1 - CI-0117 Programacion Paralela y Concurrente
// Simulacion de la ecuacion de calor 3D en estado estacionario.
// Metodo de Jacobi con estencil de 6 vecinos + paralelizacion con OpenMp
//
// Compilacion: ver Makefile (g++ -O2 -fopenmp -Wall)

#include <cstdio>    // printf, fprintf, fopen, fclose
#include <cstdlib>   // malloc, free, exit
#include <cstring>   // memcpy
#include <omp.h>     // OpenMP: directivas y omp_get_wtime

// Parametros base que pide el enunciado
#define N         100    // dimension de la malla N x N x N
#define NUM_STEPS 1000   // numero de iteraciones de Jacobi

#define TEMP_COLD  0.0     // cara inferior (i==0)
#define TEMP_HOT   100.0   // las otras cinco caras



// Conversion de coordenadas 3D (i,j,k) a indice lineal
// La malla se guarda como un bloque plano contiguo de N*N*N doubles, ya que con malloc se asigna un bloque plano de memoria
// Orden: i es el indice mas externo, k el mas interno
static inline int idx(int i, int j, int k) {
    return (i * N * N) + (j * N) + k;
}



// Asigna un arreglo 3D (bloque plano de N*N*N doubles) con malloc
// Si en el sistema no hay espacio, el programa muestra error en vez de cerrarse de forma repentina
static double* allocate_grid() {
    double* grid = (double*) malloc((size_t) N * N * N * sizeof(double));
    if (grid == NULL) {
        fprintf(stderr, "Error: no se pudo asignar memoria para la malla\n");
        exit(EXIT_FAILURE);
    }
    return grid;
}


// Libera un arreglo asignado con allocate_grid
static void free_grid(double* grid) {
    free(grid);
}


// Inicializa la malla: temperaturas iniciales. condiciones de frontera e interior.
// CONDICIONES DE FRONTERA: cara inferior (i==0) a TEMP_COLD, las otras cinco caras a TEMP_HOT
// PUNTO INTERIOR: valor promedio ponderado de las caras (5*100)
static void initialize(double* array_old, double* array_new) {
    const double interior_value =
        (5.0 * TEMP_HOT + 1.0 * TEMP_COLD) / 6.0;

    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
            for (int k = 0; k < N; k++) {
                bool is_boundary =
                    (i == 0) || (i == N - 1) ||
                    (j == 0) || (j == N - 1) ||
                    (k == 0) || (k == N - 1);

                double value;
                if (is_boundary) {
                    if (i == 0) {
                        value = TEMP_COLD;   // cara inferior
                    } else {
                        value = TEMP_HOT;    // las otras cinco caras
                    }
                } else {
                    value = interior_value;  // punto interior
                }

                array_old[idx(i, j, k)] = value;
            }
        }
    }

    // array_new arranca identico a array_old (fronteras incluidas)
    memcpy(array_new, array_old, (size_t) N * N * N * sizeof(double));
}

//jacobi_step: realiza una iteración de Jacobi
// Lee array antiguo y escribe en array nuevo, solo actualiiza puntos interiores, las fronteras no se cambian
// OpenMP: cada hilo trabaja en subconjunto de filas
// No hay condiciones de carrera porque el viejo solo se lee y el nuevo solo se escribe 
// Ningun hilo se modifica en el mismo paso

static void jacobi_step(cont double* array_old, double * array_new){
    for (int i = 1; i < N - 1; i++) {
        for (int j = 1; j < N - 1; j++) {
            for (int k = 1; k < N - 1; k++) {
                array_new[idx(i, j, k)] = (
                    array_old[idx(i - 1, j, k)] + // vecino i-1
                    array_old[idx(i + 1, j, k)] + // vecino i+1
                    array_old[idx(i, j - 1, k)] + // vecino j-1
                    array_old[idx(i, j + 1, k)] + // vecino j+1
                    array_old[idx(i, j, k - 1)] + // vecino k-1
                    array_old[idx(i, j, k + 1)]   // vecino k+1
                ) / 6.0;
            }
        }
    }
}

// write_data_for_paraview: se exporta la malla final en formato VTK legacy en ASCII
// El archivo se puede abrir directamente con Paraview 
// formatoe es "STRUCTURED_POINTS" con dimensiones N x N x N, origen (0,0,0) y espaciado (1,1,1)
static void write_data_for_paraview(const double* grid, conts char* filename){
    FILE* file = fopen(filename, "w");
    if (file == NULL) {
        fprintf(stderr, "Error: no se pudo abrir el archivo para escribir\n");
        return;
    }

    // Escribir encabezado del formato VTK legacy
    fprintf(file, "# vtk DataFile Version 3.0\n");
    fprintf(file, "Heat3D simulation result\n");
    fprintf(file, "ASCII\n");
    fprintf(file, "DATASET STRUCTURED_POINTS\n");
    fprintf(file, "DIMENSIONS %d %d %d\n", N, N, N);
    fprintf(file, "ORIGIN 0 0 0\n"); // origen fijo en (0,0,0)
    fprintf(file, "SPACING 1 1 1\n");
    fprintf(file, "POINT_DATA %d\n", N * N * N); // numero total de puntos en la malla
    fprintf(file, "SCALARS temperature double\n"); // nombre del campo de datos: "temperature", tipo double
    fprintf(file, "LOOKUP_TABLE default\n");

    // Escribir los datos de temperatura punto por punto
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
            for (int k = 0; k < N; k++) {
                fprintf(file, "%f\n", grid[idx(i, j, k)]);
            }
        }
    }

    fclose(file); 
    fprintf("Datos escritos en archivo para Paraview: %s\n", filename);
}

int main() {
    // Flujo main 
    // Asignar mem para array_old y new
    // Incializar condiciones 
    // Ejecutar n iteraciones de Jacobi (midiedno el tiempo)
    // Exportar result en archivo vtk
    // liberar memoria 

    printf("Heat3D - configuracion inicial\n");
    printf("N         = %d\n", N);
    printf("NUM_STEPS = %d\n", NUM_STEPS);

    // Asignacion de los dos arreglos de Jacobi
    double* array_old = allocate_grid();
    double* array_new = allocate_grid();

    printf("Memoria asignada correctamente para array_old y array_new\n");
    // Imprimir memoria exacta asiganda 
    printf("Memoria: 2 x %.1f MB\n", (double) N * N * N * sizeof(double) / (1024.0 * 1024.0));


    // Inicializacion: condiciones de frontera e interior
    initialize(array_old, array_new);
    printf("Malla inicializada (fronteras e interior)\n");

    //Jacobi con medicion de timepo
    printf("Iniciando simulación...\n");
    double t_start = omp_get_wtime();

    for (int step = 0; step < NUM_STEPS; step++) {
        // leer old, escribri en new
        jacobi_step(array_old, array_new);

        // copiar new a old para nueva iteracion (actualización)
        memcpy(array_old, array_new, (size_t) N * N * N * sizeof(double));
    
    }

    double t_end = omp_get_wtime();
    printf("Simulación termianda en %.4f segundos\n", t_end - t_start);

    // Resultado para paraview
    write_data_for_paraview(array_old, "heat3d_output.vtk");

    // Liberacion de memoria
    free_grid(array_old);
    free_grid(array_new);

    printf("Memoria liberada correctamente\n Fin del programa \n");
    return 0;
}