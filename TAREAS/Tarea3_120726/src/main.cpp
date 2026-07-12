// main.cpp
// pipeline: leer imagen -> escala de grises -> blur gaussiano -> guardar imagen

// compilar con: g++ -std=c++17 -O2 -I include src/main.cpp src/imagen_io.cpp -o imagenesSalida/test_read./imagenesSalida/test_read imagenesEntrada/gatito.jpg
//lectura de imagen por ahora
#include <iostream>
#include <chrono> // para medir tiempo de ejecución
#include "imagen_io.hpp"
#include "escalaGrises.hpp"
#include "gaussianBlur.hpp"

int main (int argc, char **argv) {
    std::string ruta_entrada = (argc > 1) ? argv [1]: "imagenesEntrada/gatito.jpg";

    try {
        Imagen imagen = cargarImagen(ruta_entrada);
        std::cout << "Imagen cargada: " << ruta_entrada << "\n";
        std::cout << "  Ancho:    " << imagen.ancho << " px\n";
        std::cout << "  Alto:     " << imagen.alto << " px\n";
        std::cout << "  Canales:  " << imagen.canales << "\n";
        std::cout << "  Bytes totales: " << imagen.datos.size() << "\n";

        std::vector<uint8_t> gris = convertirEscalaGrises(imagen);
        std::cout << "Imagen convertida a escala de grises. Tamaño del vector: " << gris.size() << "\n";

        // Gaussian blur en CPU con medicion de tiempo
        auto inicio = std::chrono::high_resolution_clock::now();
        std::vector<uint8_t> salida = gaussianBlurCPU(gris, imagen.ancho, imagen.alto);
        auto fin = std::chrono::high_resolution_clock::now();

        double tiempoCPUms = std::chrono::duration<double, std::milli>(fin - inicio).count();
        std::cout << "Blur gaussiano aplicado en CPU. Tiempo de ejecución: " << tiempoCPUms << " ms\n";

        // se guarda imagen en ggris para validar visualmente
        Imagen imagenGris;
        imagenGris.ancho = imagen.ancho;
        imagenGris.alto = imagen.alto;
        imagenGris.canales = 1; // escala de grises
        imagenGris.datos = gris;
        guardarImagen("imagenesSalida/gatitoGris.png", imagenGris);
        std::cout << "Imagen en escala de grises guardada en: imagenesSalida/gatitoGris.png\n";

         // se guarda imagen con blur gaussiano aplicado, para validar visualmente
        Imagen imagenBlur;
        imagenBlur.ancho = imagen.ancho;
        imagenBlur.alto = imagen.alto;
        imagenBlur.canales = 1; // escala de grises
        imagenBlur.datos = salida;
        guardarImagen("imagenesSalida/gatitoBlur_cpu.png", imagenBlur);
        std::cout << "Imagen con blur gaussiano (CPU) guardada en: imagenesSalida/gatitoBlur_cpu.png\n";

    } catch (const std::exception &e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
    return 0;
}