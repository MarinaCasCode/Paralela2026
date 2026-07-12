#include "gaussianBlur.hpp"
#include <algorithm> // para clamp, min y max

// funcion para "clamp" -> mantiene índice dentro de los límites de la imagen (0, limite-1)
static inline int clamp(int valor, int limite) {
    return std::min(std::max(valor, 0), limite - 1);
}

std::vector<uint8_t> gaussianBlurCPU(const std::vector<uint8_t> &entrada, int ancho, int alto) {
    // kernel gaussiano 3x3 normalizado, (suma total_ 16 -> se divide entre 16)
    const int kernel[3][3] = {
        {1, 2, 1},
        {2, 4, 2},
        {1, 2, 1}
    };
    const int radioKernel = 1; // radio del kernel (3x3 -> radio=1)
    const int sumaKernel = 16; // suma de los valores del kernel para normalizar

    // Array de salida APARTE del de entrada (NO se sobreescribe la imagenoriginal) 
    std::vector<uint8_t> salida(ancho * alto, 0);

    for (int y= 0; y < alto; ++y) {
        for (int x = 0; x < ancho; ++x) {
            int suma = 0;
            // aplicar el kernel gaussiano
            for (int ky = -radioKernel; ky <= radioKernel; ++ky) {
                for (int kx = -radioKernel; kx <= radioKernel; ++kx) {
                    int pixelX = clamp(x + kx, ancho);
                    int pixelY = clamp(y + ky, alto);

                    int pesoKernel = kernel[ky + radioKernel][kx + radioKernel];
                    int valorPixel = entrada[static_cast<size_t>(pixelY) * ancho + pixelX];
                    suma += valorPixel * pesoKernel;
                }
            }
            // normalizar y asignar al pixel de salida
            salida[y * ancho + x] = static_cast<uint8_t>(suma / sumaKernel);
        }
    }

    return salida;
}