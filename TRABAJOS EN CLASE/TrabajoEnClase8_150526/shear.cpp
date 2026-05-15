// Algoritmo Shear Sort con OpenMP
// Jimena Bejarano Sanchez C31074
// Marina Castro Peralta C31886
//   ./shear N            genera una matriz aleatoria de N x N
//   ./shear N fileA      lee la matriz de NxN desde el archivo fileA

#include <iostream>
#include <fstream>
#include <vector>
#include <string>
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
static bool isPowerOfTwo(int n) {
    return n > 0 && (n & (n - 1)) == 0;
}

void generateRandomMatrix(std::vector<int>& A, int N, unsigned seed) {
    A.resize(static_cast<size_t>(N) * N);
    std::srand(seed);
    for (int i = 0; i < N * N; ++i) {
        // Valores en [0, 10*N*N) para tener buena dispersión
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
            std::cerr << "Error: archivo con menos de " << N * N
                      << " elementos (leidos " << i << ")\n";
            return false;
        }
    }
    return true;
}

void printMatrix(const std::vector<int>& A, int N) {
    for (int i = 0; i < N; ++i) {
        for (int j = 0; j < N; ++j) {
            std::cout << A[i * N + j];
            if (j + 1 < N) std::cout << "\t";
        }
        std::cout << "\n";
    }
}


// Algoritmo de ordenamiento a implementar: insertion sort

// Funciones del algoritmo Shear Sort
void sortRowsAlternateDirection(std::vector<int>& A, int N) {
    // TODO
    (void)A; (void)N;
}

void sortColumns(std::vector<int>& A, int N) {
    // TODO
    (void)A; (void)N;
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
    (void)A; (void)N;
    return true;
}

int main(int argc, char* argv[]) {
    if (argc < 2 || argc > 4) {
        std::cerr << "Uso:\n"
                  << "  " << argv[0] << " N\n"
                  << "  " << argv[0] << " N fileA\n"
                  << "  " << argv[0] << " N --print          (imprime matriz)\n"
                  << "  " << argv[0] << " N fileA --print\n";
        return 1;
    }

    int N = std::atoi(argv[1]);
    if (N <= 0) {
        std::cerr << "Error: N debe ser un entero positivo\n";
        return 1;
    }
    if (!isPowerOfTwo(N)) {
        std::cerr << "Advertencia: N=" << N
                  << " no es potencia de 2. Shear Sort puede no terminar con orden perfecto.\n";
    }

    // Parsear argumentos restantes
    const char* fileA = nullptr;
    bool doPrint = false;
    for (int i = 2; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--print") {
            doPrint = true;
        } else {
            fileA = argv[i];
        }
    }

    std::vector<int> A;

    if (fileA == nullptr) {
        generateRandomMatrix(A, N, /*seed=*/42);
    } else {
        if (!readMatrixFromFile(A, N, fileA)) {
            return 1;
        }
    }

    if (doPrint) {
        std::cout << "=== Matriz inicial ===\n";
        printMatrix(A, N);
    }

    double t0 = wtime();
    shearSort(A, N);
    double t1 = wtime();

    if (doPrint) {
        std::cout << "=== Matriz final ===\n";
        printMatrix(A, N);
    }

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