//imagen_io.hpp
// Entrada y salida de imágenes 
// stb_image (lectura) y stb_image_write (escritura)

#ifndef IMAGEN_IO_HPP 
#define IMAGEN_IO_HPP // Evitar inclusiones múltiples

#include <string>
#include <vector>
#include <cstdint> // uint8_t, uint32_t, para tipos de datos enteros de tamaño fijo

// estructura para representar imagen que esta en memoria
// canales: represneta num de canales originales de la foto (1=gris, 3=RGB, 4=RGBA )
// datos: buffe de pixeles: tam= ancho * alto * canales, cada pixel es un uint8_t (0-255)
struct Imagen {
    int ancho = 0; // ancho de la imagen
    int alto = 0;  // alto de la imagen
    int canales = 0; // numero de canales (1=gris, 3=RGB, 4=RGBA)
    std::vector<uint8_t> datos; // vector de datos de la imagen (uint8_t = unsigned char)
};

// Carga de imagen desde disco (via stb_image soporta PNG, JPEG, BMP, TGA, GIF, HDR, PIC)
// Manda runtime_error si no puede cargar la imagen
Imagen cargarImagen(const std::string &ruta);

// Guarda imagen en disco en PNG
// datos deben tener tamaño ancho * alto * canales, cada pixel es un uint8_t (0-255)
void guardarImagen(const std::string &ruta, const Imagen &imagen);

#endif // IMAGEN_IO_HPP