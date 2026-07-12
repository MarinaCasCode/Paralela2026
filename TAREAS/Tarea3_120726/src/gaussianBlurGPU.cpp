#include "gaussianBlurGPU.hpp"
#include <algorithm> // std::min, std::max

std::vector<uint8_t> gaussianBlurGPU(const std::vector<uint8_t> &entrada, int ancho, int alto, int numPasadas) {
    // kernel gaussiano 3x3 normalizado, (suma total_ 16 -> se divide entre 16)
    const int kernel[3][3] = {
        {1, 2, 1},
        {2, 4, 2},
        {1, 2, 1}
    };
    const int radioKernel = 1; // radio del kernel (3x3 -> radio=1)
    const int sumaKernel = 16; // suma de los valores del kernel para normalizar

    const size_t numPixeles = static_cast<size_t>(ancho) * static_cast<size_t>(alto);

    // dos buffers que se turnan entre pasadas (ping-pong), quedan en la GPU toda la funcion
    std::vector<uint8_t> bufA = entrada;        // copia inicial de la entrada
    std::vector<uint8_t> bufB(numPixeles, 0);   // buffer de trabajo

    // Punteros: OpenCC trabaja mejor co arreglos "planos" o bidimensionales en memoria
    // que con vector dentro de vector, por eso se usan punteros a los datos del vector
    uint8_t *ptrA = bufA.data();
    uint8_t *ptrB = bufB.data();

    // #pragma acc data:
    // Aquí se usa OpenACC para paralelizar el bucle externo (y) y el bucle interno (x)
    // copyin(ptrA[0:numPixeles]) // copiar datos de entrada a la GPU, una sola vez
    // create(ptrB[0:numPixeles]) // reservar el buffer de trabajo en la GPU, sin transferir
    #pragma acc data copyin(ptrA[0:numPixeles]) create(ptrB[0:numPixeles])
    {
        // nuevo: todas las pasadas viven adentro de esta region acc data,
        // asi los buffers no se transfieren de vuelta a CPU entre pasadas
        for (int pasada = 0; pasada < numPasadas; ++pasada) {
            uint8_t *entradaPtr = (pasada % 2 == 0) ? ptrA : ptrB;
            uint8_t *salidaPtr  = (pasada % 2 == 0) ? ptrB : ptrA;

            // pragama: colapsar bucles y y x para paralelizar, en un solo esapcio de iteracion
            // para que la GPU reparta cada pixel existente entre sus hilos disponibles
            #pragma acc parallel loop collapse(2) present(entradaPtr[0:numPixeles], salidaPtr[0:numPixeles])
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
        }

        // nuevo: el resultado final queda en A si numPasadas es par, o en B si es impar,
        // aca se trae de vuelta a CPU una sola vez, al terminar todas las pasadas
        if (numPasadas % 2 == 0) {
            #pragma acc update self(ptrA[0:numPixeles])
        } else {
            #pragma acc update self(ptrB[0:numPixeles])
        }
    } // fin de la region acc data

    return (numPasadas % 2 == 0) ? bufA : bufB;
}