// Algoritmo de Strassen Escriba un programa paralelo con directivas de OpenMP que implemente el algoritmo de Strassen para multiplicación de matrices
// Jimena Bejarano Sanchez C31074
// Marina Castro Peralta C31886

// compilar con OpenMP: g++ strassen.cpp -o strassen -fopenmp -std=c++17
// compilar sin OpenMP: g++ strassen.cpp -o strassen -std=c++17

#include <iostream>
#include <cstdlib> // para usar rand() y srand()
#include <ctime> // para usar time(0) y generar matrices aleatorias
#include <omp.h>

using namespace std;

const int N = 4; // tamaño N de las matrices

// suma dos matrices de tamaño n y guarda el resultado en resultado
void sumar(int resultado[N][N], int izq[N][N], int der[N][N], int n) {
    for (int fila = 0; fila < n; fila++)
        for (int col = 0; col < n; col++)
            resultado[fila][col] = izq[fila][col] + der[fila][col];
}

// resta dos matrices de tamaño n y guarda el resultado en resultado
void restar(int resultado[N][N], int izq[N][N], int der[N][N], int n) {
    for (int fila = 0; fila < n; fila++)
        for (int col = 0; col < n; col++)
            resultado[fila][col] = izq[fila][col] - der[fila][col];
}

void strassen(int C[N][N], int A[N][N], int B[N][N], int n) {
    // caso base: multiplicacion directa 2x2
    if (n == 2) {
        for (int i = 0; i < 2; i++)
            for (int j = 0; j < 2; j++) {
                C[i][j] = 0;
                for (int k = 0; k < 2; k++)
                    C[i][j] += A[i][k] * B[k][j];
            }
        return;
    }
    int mitad = n / 2;

    // submatrices de A y B
    int A11[N][N]={};
    int A12[N][N]={};
    int A21[N][N]={};
    int A22[N][N]={};
    int B11[N][N]={};
    int B12[N][N]={};
    int B21[N][N]={};
    int B22[N][N]={};

    // dividir A y B en cuadrantes
    for (int fila = 0; fila < mitad; fila++) {
        for (int col = 0; col < mitad; col++) {
            A11[fila][col] = A[fila][col];
            A12[fila][col] = A[fila][col+mitad];
            A21[fila][col] = A[fila+mitad][col];
            A22[fila][col] = A[fila+mitad][col+mitad];
            B11[fila][col] = B[fila][col];
            B12[fila][col] = B[fila][col+mitad];
            B21[fila][col] = B[fila+mitad][col];
            B22[fila][col] = B[fila+mitad][col+mitad];
        }
    }

    // los 7 productos de Strassen
    int P1[N][N]={};
    int P2[N][N]={};
    int P3[N][N]={};
    int P4[N][N]={};
    int P5[N][N]={};
    int P6[N][N]={};
    int P7[N][N]={};

    // calcular los 7 productos en paralelo
    #pragma omp parallel sections
    {
        #pragma omp section
        {
            int temp1[N][N]={};
            int temp2[N][N]={};
            sumar(temp1, A11, A22, mitad);
            sumar(temp2, B11, B22, mitad);
            strassen(P1, temp1, temp2, mitad);  // P1 = (A11+A22)(B11+B22)
        }
        #pragma omp section
        {
            int temp1[N][N]={};
            sumar(temp1, A21, A22, mitad);
            strassen(P2, temp1, B11, mitad);    // P2 = (A21+A22)*B11
        }
        #pragma omp section
        {
            int temp1[N][N]={};
            restar(temp1, B12, B22, mitad);
            strassen(P3, A11, temp1, mitad);    // P3 = A11*(B12-B22)
        }
        #pragma omp section
        {
            int temp1[N][N]={};
            restar(temp1, B21, B11, mitad);
            strassen(P4, A22, temp1, mitad);    // P4 = A22*(B21-B11)
        }
        #pragma omp section
        {
            int temp1[N][N]={};
            sumar(temp1, A11, A12, mitad);
            strassen(P5, temp1, B22, mitad);    // P5 = (A11+A12)*B22
        }
        #pragma omp section
        {
            int temp1[N][N]={};
            int temp2[N][N]={};
            restar(temp1, A21, A11, mitad);
            sumar(temp2, B11, B12, mitad);
            strassen(P6, temp1, temp2, mitad);  // P6 = (A21-A11)(B11+B12)
        }
        #pragma omp section
        {
            int temp1[N][N]={};
            int temp2[N][N]={};
            restar(temp1, A12, A22, mitad);
            sumar(temp2, B21, B22, mitad);
            strassen(P7, temp1, temp2, mitad);  // P7 = (A12-A22)(B21+B22)
        }
    }

    // combinar los productos para obtener los 4 cuadrantes del resultado
    #pragma omp parallel sections
    {
        #pragma omp section
        {
            for (int fila = 0; fila < mitad; fila++)
                for (int col = 0; col < mitad; col++)
                    C[fila][col] = P1[fila][col] + P4[fila][col] - P5[fila][col] + P7[fila][col];
        }
        #pragma omp section
        {
            for (int fila = 0; fila < mitad; fila++)
                for (int col = 0; col < mitad; col++)
                    C[fila][col+mitad] = P3[fila][col] + P5[fila][col];
        }
        #pragma omp section
        {
            for (int fila = 0; fila < mitad; fila++)
                for (int col = 0; col < mitad; col++)
                    C[fila+mitad][col] = P2[fila][col] + P4[fila][col];
        }
        #pragma omp section
        {
            for (int fila = 0; fila < mitad; fila++)
                for (int col = 0; col < mitad; col++)
                    C[fila+mitad][col+mitad] = P1[fila][col] - P2[fila][col] + P3[fila][col] + P6[fila][col];
        }
    }
}

int main() {
    srand(time(0));
    int A[N][N], B[N][N], resultado[N][N] = {};
    for (int i = 0; i < N; i++)
        for (int j = 0; j < N; j++) {
            A[i][j] = rand() % 9 + 1;
            B[i][j] = rand() % 9 + 1;
        }

    strassen(resultado, A, B, N);

    cout << "Resultado de la multiplicacion de matrices:\n";
    for (int fila = 0; fila < N; fila++) {
        for (int col = 0; col < N; col++)
            cout << resultado[fila][col] << " ";
        cout << "\n";
    }
    return 0;
}
