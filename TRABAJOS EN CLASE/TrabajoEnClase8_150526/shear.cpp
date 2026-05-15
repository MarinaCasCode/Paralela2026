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


// Insertion sort genérico con stride
static void insertionSortStrided(std::vector<int>& A,
                                 int start, int count, int stride,
                                 bool ascending) {
    for (int i = 1; i < count; ++i) {
        int key = A[start + i * stride];
        int j = i - 1;
        if (ascending) {
            while (j >= 0 && A[start + j * stride] > key) {
                A[start + (j + 1) * stride] = A[start + j * stride];
                --j;
            }
        } else {
            while (j >= 0 && A[start + j * stride] < key) {
                A[start + (j + 1) * stride] = A[start + j * stride];
                --j;
            }
        }
        A[start + (j + 1) * stride] = key;
    }
}


// Funciones del algoritmo Shear Sort
void sortRowsAlternateDirection(std::vector<int>& A, int N) {
    // Fila i empieza en A[i*N], tiene N elementos, stride = 1.
    // Filas pares (i % 2 == 0) -> ascendente
    // Filas impares           -> descendente
    for (int i = 0; i < N; ++i) {
        bool ascending = (i % 2 == 0);
        insertionSortStrided(A, /*start=*/i * N, /*count=*/N,
                             /*stride=*/1, ascending);
    }
}

void sortColumns(std::vector<int>& A, int N) {
    // Columna j empieza en A[j], tiene N elementos, stride = N.
    // Todas las columnas en orden ascendente.
    for (int j = 0; j < N; ++j) {
        insertionSortStrided(A, /*start=*/j, /*count=*/N,
                             /*stride=*/N, /*ascending=*/true);
    }
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