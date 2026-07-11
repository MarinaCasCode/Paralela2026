// main.cpp
// pipeline: leer imagen -> escala de grises -> blur gaussiano -> guardar imagen

// compilar con: g++ -std=c++17 -O2 -I include src/main.cpp src/imagen_io.cpp -o imagenesSalida/test_read./imagenesSalida/test_read imagenesEntrada/gatito.jpg
//lectura de imagen por ahora
#include <iostream>
#include "imagen_io.hpp"

int main (int argc, char **argv) {
    std::string ruta_entrada = (argc > 1) ? argv [1]: "imagenesEntrada/gatito.jpg";

    try {
        Imagen imagen = cargarImagen(ruta_entrada);
        std::cout << "Imagen cargada: " << ruta_entrada << "\n";
        std::cout << "  Ancho:    " << imagen.ancho << " px\n";
        std::cout << "  Alto:     " << imagen.alto << " px\n";
        std::cout << "  Canales:  " << imagen.canales << "\n";
        std::cout << "  Bytes totales: " << imagen.datos.size() << "\n";
    } catch (const std::exception &e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
    return 0;
}