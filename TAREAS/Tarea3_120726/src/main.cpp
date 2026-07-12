// main.cpp
// pipeline: leer imagen -> escala de grises -> blur gaussiano -> guardar imagen

// compilar con: g++ -std=c++17 -O2 -I include src/main.cpp src/imagen_io.cpp -o imagenesSalida/test_read./imagenesSalida/test_read imagenesEntrada/gatito.jpg
//lectura de imagen por ahora
#include <iostream>
#include <chrono> // para medir tiempo de ejecución
#include "imagen_io.hpp"
#include "escalaGrises.hpp"
#include "gaussianBlur.hpp"
#include "gaussianBlurGPU.hpp"  

int main (int argc, char **argv) {
    std::string ruta_entrada = (argc > 1) ? argv [1]: "imagenesEntrada/gatito.jpg";
    

    // Numero de veces que se aplica el kernel 3x3 
    // Cada pasada difumina un poco mas (mismo kernel, aplicado repetidamente)
    const int numIteraciones = 60;

    try {
        Imagen imagen = cargarImagen(ruta_entrada);
        std::cout << "Imagen cargada: " << ruta_entrada << "\n";
        std::cout << "  Ancho:    " << imagen.ancho << " px\n";
        std::cout << "  Alto:     " << imagen.alto << " px\n";
        std::cout << "  Canales:  " << imagen.canales << "\n";
        std::cout << "  Bytes totales: " << imagen.datos.size() << "\n";

        std::vector<uint8_t> gris = convertirEscalaGrises(imagen);
        std::cout << "Imagen convertida a escala de grises. Tamaño del vector: " << gris.size() << "\n";

        // Gaussian blur en CPU, aplicado numIteraciones veces,
        // con medicion del tiempo TOTAL de las iteraciones.
        auto inicioCPU = std::chrono::high_resolution_clock::now();

        std::vector<uint8_t> salida = gris; // punto de partida: la imagen en gris
        for (int i = 0; i < numIteraciones; ++i) {
            salida = gaussianBlurCPU(salida, imagen.ancho, imagen.alto);
        }

        auto finCPU = std::chrono::high_resolution_clock::now();

    
        double tiempoCPUms = std::chrono::duration<double, std::milli>(finCPU - inicioCPU).count();
        std::cout << "Blur gaussiano aplicado en CPU (" << numIteraciones << " iteraciones). Tiempo de ejecución: " << tiempoCPUms << " ms\n";

        // Gaussian blur en GPU, aplicado numIteraciones veces,
        // con medicion del tiempo TOTAL de las iteraciones.
        auto inicioGPU = std::chrono::high_resolution_clock::now();

        std::vector<uint8_t> salidaGPU = gris;
        for (int i = 0; i < numIteraciones; ++i) {
            salidaGPU = gaussianBlurGPU(salidaGPU, imagen.ancho, imagen.alto);
        }

        auto finGPU = std::chrono::high_resolution_clock::now();
        double tiempoGPUms = std::chrono::duration<double, std::milli>(finGPU - inicioGPU).count();
        std::cout << "Blur gaussiano GPU (" << numIteraciones
                  << " pasadas). Tiempo: " << tiempoGPUms << " ms\n";

        // Speedup: tiempoCPU / tiempoGPU
        double speedup = tiempoCPUms / tiempoGPUms;
        std::cout << "Speedup (CPU/GPU): " << speedup << "\n";

        //GUARDAR IMAGENES
        // se guarda imagen en ggris para validar visualmente
        Imagen imagenGris;
        imagenGris.ancho = imagen.ancho;
        imagenGris.alto = imagen.alto;
        imagenGris.canales = 1; // escala de grises
        imagenGris.datos = gris;
        guardarImagen("imagenesSalida/gatitoGris.png", imagenGris);
        std::cout << "Imagen en escala de grises guardada en: imagenesSalida/gatitoGris.png\n";

         // se guarda imagen con blur gaussiano aplicado en CPU, para validar visualmente
        Imagen imagenBlur_CPU; // no se guarda como imagenBlurCPU para evitar confusión con la función gaussianBlurCPU
        imagenBlur_CPU.ancho = imagen.ancho;
        imagenBlur_CPU.alto = imagen.alto;
        imagenBlur_CPU.canales = 1; // escala de grises
        imagenBlur_CPU.datos = salida;
        guardarImagen("imagenesSalida/gatitoBlurCPU.png", imagenBlur_CPU);
        std::cout << "Imagen con blur gaussiano (CPU) guardada en: imagenesSalida/gatitoBlurCPU.png\n";

        // se guarda imagen con blur gaussiano aplicado en GPU, para validar visualmente
        Imagen imagenBlur_GPU; // no se guarad como imagenBlurGPU para evitar confusión con la función gaussianBlurGPU
        imagenBlur_GPU.ancho = imagen.ancho;
        imagenBlur_GPU.alto = imagen.alto;
        imagenBlur_GPU.canales = 1;
        imagenBlur_GPU.datos = salidaGPU;
        guardarImagen("imagenesSalida/gatitoBlurGPU.png", imagenBlur_GPU);
        std::cout << "Imagen con blur (GPU) guardada en: imagenesSalida/gatitoBlurGPU.png\n";

    } catch (const std::exception &e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
    return 0;
}