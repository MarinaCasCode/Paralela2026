#include "gaussianBlurGPU.hpp"
#include <algorithm> // std::min, std::max

std::vector<uint8_t> gaussianBlurGPU(const std::vector<uint8_t> &entrada, int ancho, int alto) {
    // kernel gaussiano 3x3 normalizado, (suma total_ 16 -> se divide entre 16)
    const int kernel[3][3] = {
        {1, 2, 1},
        {2, 4, 2},
        {1, 2, 1}
    };
    const int radioKernel = 1; // radio del kernel (3x3 -> radio=1)
    const int sumaKernel = 16; // suma de los valores del kernel para normalizar

    const size_t numPixeles = static_cast<size_t>(ancho) * static_cast<size_t>(alto);

    // Arrat de salida APARTE del de entrada (NO se sobreescribe la imagenoriginal)
    std::vector<uint8_t> salida(numPixeles, 0);

    // Punteros: OpenCC trabaja mejor co arreglos "planos" o bidimensionales en memoria
    // que con vector dentro de vector, por eso se usan punteros a los datos del vector
    const uint8_t *entradaPtr = entrada.data();
    uint8_t *salidaPtr = salida.data();

    // #pragma acc data:
    // Aquí se usa OpenACC para paralelizar el bucle externo (y) y el bucle interno (x)
    // copyIn(entradaPtr[0:numPixeles]) // copiar datos de entrada a la GPU
    // copyOut(salidaPtr[0:numPixeles]) // copiar datos de salida de la GPU a la CPU
    #pragma acc data copyIn(entradaPtr[0:numPixeles]) copyOut(salidaPtr[0:numPixeles])
    {
        // pragama: colapsar bucles y y x para paralelizar, en un solo esapcio de iteracion
        // para que la GPU reparta cada pixel existente entre sus hilos disponibles
        #pragma acc parallel loop collapse(2) // colapsar bucles y y x para paralelizar
        for (int y = 0; y < alto; ++y) {
            for (int x = 0; x < ancho; ++x) {
                int suma = 0;


                // aplicar el kernel gaussiano
                // copia de valores constantes (no se comparte el estado) 
                for (int ky = -radioKernel; ky <= radioKernel; ++ky) {
                    for (int kx = -radioKernel; kx <= radioKernel; ++kx) {
                        // clamp manual: inline para no llamar a funcion externa 
                        // para no hacer acc routine y que la GPU pueda optimizar mejor
                        // y que se pueda llamar dentro de la reggión paralela
                        int pixelY = y + ky;
                        pixelY = (pixelY < 0) ? 0 : ((pixelY >= alto) ? alto - 1 : pixelY); // clamp

                        int pixelX = x + kx;
                        pixelX = (pixelX < 0) ? 0 : ((pixelX >= ancho) ? ancho - 1 : pixelX); // clamp

                    
                        int pesoKernel = kernel[ky + radioKernel][kx + radioKernel];
                        int valorPixel = entradaPtr[static_cast<size_t>(pixelY) * ancho + pixelX];
                        suma += valorPixel * pesoKernel;
                    }
                }
                salidaPtr[static_cast<size_t>(y) * ancho + x] = static_cast<uint8_t>(suma / sumaKernel);    // normalizar y asignar al pixel de salida
            }   
        }
    } // fin de la region acc data

    return salida;
}