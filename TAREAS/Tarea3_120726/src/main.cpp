// main.cpp
// pipeline: leer imagen -> escala de grises -> blur gaussiano -> guardar imagen
// Incluye un menu interactivo para elegir la imagen de entrada.

#include <iostream>
#include <iomanip>
#include <chrono>
#include <filesystem>
#include <vector>
#include <algorithm>
#include "imagen_io.hpp"
#include "escalaGrises.hpp"
#include "gaussianBlur.hpp"
#include "gaussianBlurGPU.hpp"

namespace fs = std::filesystem;

// Funcion aux para imprimir separador de seccion en terminal
void imprimirSeparador(const std::string &titulo) {
    std::cout << "\n============================================\n";
    std::cout << " " << titulo << "\n";
    std::cout << "============================================\n";
}

// Extensiones de imagen que aceptamos mostrar en el menu (soportadas por stb_image)
bool esExtensionDeImagen(const fs::path &ruta) {
    std::string ext = ruta.extension().string();
    // pasar a minusculas para comparar sin importar mayusculas/minusculas
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    return ext == ".png" || ext == ".jpg" || ext == ".jpeg" ||
           ext == ".bmp" || ext == ".tga";
}

// Escanea la carpeta dada y devuelve la lista de rutas de imagenes encontradas
std::vector<fs::path> listarImagenesDisponibles(const std::string &carpeta) {
    std::vector<fs::path> imagenes;

    for (const auto &entrada : fs::directory_iterator(carpeta)) {
        if (entrada.is_regular_file() && esExtensionDeImagen(entrada.path())) {
            imagenes.push_back(entrada.path());
        }
    }

    // orden alfabetico para que el menu sea consistente cada vez
    std::sort(imagenes.begin(), imagenes.end());
    return imagenes;
}

// Muestra el menu numerado y devuelve la ruta elegida por el usuario
std::string elegirImagenPorMenu(const std::vector<fs::path> &imagenes) {
    imprimirSeparador("SELECCION DE IMAGEN DE ENTRADA");

    for (size_t i = 0; i < imagenes.size(); ++i) {
        std::cout << "  [" << (i + 1) << "] " << imagenes[i].filename().string() << "\n";
    }

    int opcion = 0;
    while (true) {
        std::cout << "\nElige el numero de la imagen a procesar: ";
        std::cin >> opcion;

        if (std::cin.fail()) {
            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            std::cout << "  Entrada invalida, escribe un numero.\n";
            continue;
        }

        if (opcion >= 1 && static_cast<size_t>(opcion) <= imagenes.size()) {
            break;
        }
        std::cout << "  Numero fuera de rango, intenta de nuevo.\n";
    }

    return imagenes[static_cast<size_t>(opcion - 1)].string();
}

int main (int argc, char **argv) {
    const std::string carpetaEntrada = "imagenesEntrada";
    const int numIteraciones = 60;

    std::cout << std::fixed << std::setprecision(2);

    std::string ruta_entrada;

    try {
        // Si el usuario pasa la ruta como argumento por linea de comandos,
        // se respeta eso y se salta el menu (util para pruebas rapidas).
        if (argc > 1) {
            ruta_entrada = argv[1];
        } else {
            std::vector<fs::path> imagenes = listarImagenesDisponibles(carpetaEntrada);

            if (imagenes.empty()) {
                std::cerr << "No se encontraron imagenes en la carpeta '"
                          << carpetaEntrada << "'.\n";
                return 1;
            }

            ruta_entrada = elegirImagenPorMenu(imagenes);
        }

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
        auto inicioCPU = std::chrono::high_resolution_clock::now();

        std::vector<uint8_t> salida = gris;
        for (int i = 0; i < numIteraciones; ++i) {
            salida = gaussianBlurCPU(salida, imagen.ancho, imagen.alto);
        }

        auto finCPU = std::chrono::high_resolution_clock::now();
        double tiempoCPUms = std::chrono::duration<double, std::milli>(finCPU - inicioCPU).count();
        std::cout << "  Pasadas:  " << numIteraciones << "\n";
        std::cout << "  Tiempo:   " << tiempoCPUms << " ms\n";

        imprimirSeparador("BLUR GAUSSIANO - GPU (OpenACC)");
        auto inicioGPU = std::chrono::high_resolution_clock::now();

        std::vector<uint8_t> salidaGPU = gaussianBlurGPU(gris, imagen.ancho, imagen.alto, numIteraciones);

        auto finGPU = std::chrono::high_resolution_clock::now();
        double tiempoGPUms = std::chrono::duration<double, std::milli>(finGPU - inicioGPU).count();
        std::cout << "  Pasadas:  " << numIteraciones << "\n";
        std::cout << "  Tiempo:   " << tiempoGPUms << " ms\n";

        imprimirSeparador("COMPARACION DE RENDIMIENTO");
        double speedup = tiempoCPUms / tiempoGPUms;
        std::cout << "  Tiempo CPU:  " << std::setw(10) << tiempoCPUms << " ms\n";
        std::cout << "  Tiempo GPU:  " << std::setw(10) << tiempoGPUms << " ms\n";
        std::cout << "  Speedup:     " << std::setw(10) << speedup << "x\n";

        imprimirSeparador("GUARDANDO IMAGENES DE SALIDA");

        // Usamos el nombre base del archivo elegido, para no pisar resultados
        // si se procesan varias imagenes distintas en la misma carpeta de salida.
        std::string nombreBase = fs::path(ruta_entrada).stem().string();

        Imagen imagenGris;
        imagenGris.ancho = imagen.ancho;
        imagenGris.alto = imagen.alto;
        imagenGris.canales = 1;
        imagenGris.datos = gris;
        std::string rutaGris = "imagenesSalida/" + nombreBase + "_gris.png";
        guardarImagen(rutaGris, imagenGris);
        std::cout << "  [OK] " << rutaGris << "\n";

        Imagen imagenBlur_CPU;
        imagenBlur_CPU.ancho = imagen.ancho;
        imagenBlur_CPU.alto = imagen.alto;
        imagenBlur_CPU.canales = 1;
        imagenBlur_CPU.datos = salida;
        std::string rutaBlurCPU = "imagenesSalida/" + nombreBase + "_blurCPU.png";
        guardarImagen(rutaBlurCPU, imagenBlur_CPU);
        std::cout << "  [OK] " << rutaBlurCPU << "\n";

        Imagen imagenBlur_GPU;
        imagenBlur_GPU.ancho = imagen.ancho;
        imagenBlur_GPU.alto = imagen.alto;
        imagenBlur_GPU.canales = 1;
        imagenBlur_GPU.datos = salidaGPU;
        std::string rutaBlurGPU = "imagenesSalida/" + nombreBase + "_blurGPU.png";
        guardarImagen(rutaBlurGPU, imagenBlur_GPU);
        std::cout << "  [OK] " << rutaBlurGPU << "\n";

        std::cout << "\n============================================\n";
        std::cout << " PIPELINE COMPLETADO EXITOSAMENTE\n";
        std::cout << "============================================\n\n";

    } catch (const std::exception &e) {
        std::cerr << "\n[ERROR] " << e.what() << "\n";
        return 1;
    }
    return 0;
}