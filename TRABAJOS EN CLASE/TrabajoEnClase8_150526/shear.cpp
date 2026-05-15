// Algoritmo Shear Sort con OpenMP
// Jimena Bejarano Sanchez C31074
// Marina Castro Peralta C31886
//   ./shear N            genera una matriz aleatoria de N x N
//   ./shear N fileA      lee la matriz de NxN desde el archivo fileA

#include <iostream>
#include <fstream>
#include <vector>
#include <cstdlib>
#include <cstdio>
#include <cmath>
#include <ctime>
#include <chrono>

#ifdef _OPENMP
#include <omp.h>
#endif

// Utilidades de tiempo (funcionan con o sin OpenMP)
static double wtime() {
#ifdef _OPENMP
    return omp_get_wtime();
#else
    using namespace std::chrono;
    return duration<double>(steady_clock::now().time_since_epoch()).count();
#endif
}

// Manejo de la matriz
void generateRandomMatrix(std::vector<int>& A, int N) {
    A.resize(static_cast<size_t>(N) * N);
    for (int i = 0; i < N * N; ++i) {
        A[i] = std::rand() % (N * N * 10);
    }
}

bool readMatrixFromFile(std::vector<int>& A, int N, const char* filename) {
    std::ifstream in(filename);
    if (!in.is_open()) {
        std::cerr << "Error: no se pudo abrir el archivo " << filename << "\n";
        return false;
    }
    A.resize(static_cast<size_t>(N) * N);
    for (int i = 0; i < N * N; ++i) {
        if (!(in >> A[i])) {
            std::cerr << "Error: archivo con menos de " << N * N << " elementos\n";
            return false;
        }
    }
    return true;
}

void printMatrix(const std::vector<int>& A, int N) {
    for (int i = 0; i < N; ++i) {
        for (int j = 0; j < N; ++j) {
            std::cout << A[i * N + j] << " ";
        }
        std::cout << "\n";
    }
}


// Algoritmo de ordenamiento a implementar: insertion sort

// Funciones del algoritmo Shear Sort
void sortRowsAlternateDirection(std::vector<int>& A, int N) {
    // TODO
}

void sortColumns(std::vector<int>& A, int N) {
    // TODO
}

void shearSort(std::vector<int>& A, int N) {
    int steps = static_cast<int>(std::ceil(std::log2(static_cast<double>(N) * N)));
    for (int s = 0; s < steps; ++s) {
        sortRowsAlternateDirection(A, N);
        sortColumns(A, N);
    }
}

bool verifySnakeOrder(const std::vector<int>& A, int N) {
    // TODO
    return true;
}

int main(int argc, char* argv[]) {
    if (argc < 2 || argc > 3) {
        std::cerr << "Uso:\n"
                  << "  " << argv[0] << " N\n"
                  << "  " << argv[0] << " N fileA\n";
        return 1;
    }

    int N = std::atoi(argv[1]);
    if (N <= 0) {
        std::cerr << "Error: N debe ser un entero positivo\n";
        return 1;
    }

    std::vector<int> A;

    if (argc == 2) {
        std::srand(static_cast<unsigned>(std::time(nullptr)));
        generateRandomMatrix(A, N);
    } else {
        if (!readMatrixFromFile(A, N, argv[2])) {
            return 1;
        }
    }

    double t0 = wtime();
    shearSort(A, N);
    double t1 = wtime();

    std::cout << "N = " << N << "\n";
#ifdef _OPENMP
    std::cout << "Threads = " << omp_get_max_threads() << "\n";
#else
    std::cout << "Threads = 1 (secuencial)\n";
#endif
    std::cout << "Tiempo = " << (t1 - t0) << " s\n";

    bool ok = verifySnakeOrder(A, N);
    std::cout << "Resultado " << (ok ? "OK" : "INCORRECTO") << "\n";

    return ok ? 0 : 2;
}