// main.cpp
// pipeline: leer imagen -> escala de grises -> blur gaussiano -> guardar imagen

#include <iostream>
#include <iomanip>
#include <chrono> // para medir tiempo de ejecución
#include "imagen_io.hpp"
#include "escalaGrises.hpp"
#include "gaussianBlur.hpp"
#include "gaussianBlurGPU.hpp"

// Funcion aux para imprimir separador de seccion en terminal
void imprimirSeparador(const std::string &titulo) {
    std::cout << "\n============================================\n";
    std::cout << " " << titulo << "\n";
    std::cout << "============================================\n";
}

int main (int argc, char **argv) {
    std::string ruta_entrada = (argc > 1) ? argv[1] : "imagenesEntrada/gatito.jpg";
    // Numero de veces que se aplica el kernel 3x3
    // Cada pasada difumina un poco mas (mismo kernel, aplicado repetidamente)
    const int numIteraciones = 60;

    // Numeros con 2 decimales fijos, para que se vean ordenados
    std::cout << std::fixed << std::setprecision(2);

    try {
        imprimirSeparador("LECTURA DE IMAGEN");
        Imagen imagen = cargarImagen(ruta_entrada);
        std::cout << "  Archivo:  " << ruta_entrada << "\n";
        std::cout << "  Ancho:    " << imagen.ancho << " px\n";
        std::cout << "  Alto:     " << imagen.alto << " px\n";
        std::cout << "  Canales:  " << imagen.canales << "\n";
        std::cout << "  Bytes:    " << imagen.datos.size() << "\n";

        imprimirSeparador("CONVERSION A ESCALA DE GRISES");
        std::vector<uint8_t> gris = convertirEscalaGrises(imagen);
        std::cout << "  Pixeles procesados: " << gris.size() << "\n";

        imprimirSeparador("BLUR GAUSSIANO - CPU");
        // Gaussian blur en CPU, aplicado numIteraciones veces,
        // con medicion del tiempo TOTAL de las iteraciones.
        auto inicioCPU = std::chrono::high_resolution_clock::now();

        std::vector<uint8_t> salida = gris; // punto de partida: la imagen en gris
        for (int i = 0; i < numIteraciones; ++i) {
            salida = gaussianBlurCPU(salida, imagen.ancho, imagen.alto);
        }

        auto finCPU = std::chrono::high_resolution_clock::now();
        double tiempoCPUms = std::chrono::duration<double, std::milli>(finCPU - inicioCPU).count();
        std::cout << "  Pasadas:  " << numIteraciones << "\n";
        std::cout << "  Tiempo:   " << tiempoCPUms << " ms\n";

        imprimirSeparador("BLUR GAUSSIANO - GPU (OpenACC)");
        // Gaussian blur en GPU, aplicado numIteraciones veces,
        // con medicion del tiempo TOTAL de las iteraciones.
        auto inicioGPU = std::chrono::high_resolution_clock::now();

        std::vector<uint8_t> salidaGPU = gris;
        for (int i = 0; i < numIteraciones; ++i) {
            salidaGPU = gaussianBlurGPU(salidaGPU, imagen.ancho, imagen.alto);
        }

        auto finGPU = std::chrono::high_resolution_clock::now();
        double tiempoGPUms = std::chrono::duration<double, std::milli>(finGPU - inicioGPU).count();
        std::cout << "  Pasadas:  " << numIteraciones << "\n";
        std::cout << "  Tiempo:   " << tiempoGPUms << " ms\n";

        imprimirSeparador("COMPARACION DE RENDIMIENTO");
        // Speedup: tiempoCPU / tiempoGPU
        double speedup = tiempoCPUms / tiempoGPUms;
        std::cout << "  Tiempo CPU:  " << std::setw(10) << tiempoCPUms << " ms\n";
        std::cout << "  Tiempo GPU:  " << std::setw(10) << tiempoGPUms << " ms\n";
        std::cout << "  Speedup:     " << std::setw(10) << speedup << "x\n";

        imprimirSeparador("GUARDANDO IMAGENES DE SALIDA");

        // se guarda imagen en gris para validar visualmente
        Imagen imagenGris;
        imagenGris.ancho = imagen.ancho;
        imagenGris.alto = imagen.alto;
        imagenGris.canales = 1; // escala de grises
        imagenGris.datos = gris;
        guardarImagen("imagenesSalida/gatitoGris.png", imagenGris);
        std::cout << "  [OK] imagenesSalida/gatitoGris.png\n";

        // se guarda imagen con blur gaussiano aplicado en CPU, para validar visualmente
        Imagen imagenBlur_CPU; // no se guarda como imagenBlurCPU para evitar confusión con la función gaussianBlurCPU
        imagenBlur_CPU.ancho = imagen.ancho;
        imagenBlur_CPU.alto = imagen.alto;
        imagenBlur_CPU.canales = 1; // escala de grises
        imagenBlur_CPU.datos = salida;
        guardarImagen("imagenesSalida/gatitoBlurCPU.png", imagenBlur_CPU);
        std::cout << "  [OK] imagenesSalida/gatitoBlurCPU.png\n";

        // se guarda imagen con blur gaussiano aplicado en GPU, para validar visualmente
        Imagen imagenBlur_GPU; // no se guarda como imagenBlurGPU para evitar confusión con la función gaussianBlurGPU
        imagenBlur_GPU.ancho = imagen.ancho;
        imagenBlur_GPU.alto = imagen.alto;
        imagenBlur_GPU.canales = 1;
        imagenBlur_GPU.datos = salidaGPU;
        guardarImagen("imagenesSalida/gatitoBlurGPU.png", imagenBlur_GPU);
        std::cout << "  [OK] imagenesSalida/gatitoBlurGPU.png\n";

        std::cout << "\n============================================\n";
        std::cout << " PIPELINE COMPLETADO EXITOSAMENTE\n";
        std::cout << "============================================\n\n";

    } catch (const std::exception &e) {
        std::cerr << "\n[ERROR] " << e.what() << "\n";
        return 1;
    }
    return 0;
}