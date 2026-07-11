// imagen_io.cpp
// implementacion de las salidas de las funciones de entrada y salida de imagenes 
// de stb_image y stb_image_write

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h" // para cargar imagenes
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h" // para guardar imagenes

#include "imagen_io.hpp"
#include <stdexcept> // std::runtime_error

Imagen cargarImagen(const std::string &ruta) {
    Imagen imagen;
    // Cargar imagen usando stb_image
    unsigned char *data = stbi_load(ruta.c_str(), &imagen.ancho, &imagen.alto, &imagen.canales, 0); // mantiene num original de canales
    if (!data) {
        throw std::runtime_error("Error al cargar la imagen: " + ruta + " - " + stbi_failure_reason() + " - ");
    }
    size_t tamano = static_cast<size_t>(imagen.ancho) * static_cast<size_t>(imagen.alto) * static_cast<size_t>(imagen.canales);
    // Copiar datos a vector
    imagen.datos.assign(data, data + tamano);
    stbi_image_free(data); // liberar memoria de stb_image
    return imagen;

}

void guardarImagen(const std::string &ruta, const Imagen &imagen) {
    // Guardar imagen usando stb_image_write en formato PNG
    int progresoBytes = imagen.ancho * imagen.canales; // stride en bytes
    int bien = stbi_write_png(ruta.c_str(), imagen.ancho, imagen.alto, imagen.canales, imagen.datos.data(), progresoBytes);
    if (!bien) {
        throw std::runtime_error("Error al guardar la imagen: " + ruta);
    }
}