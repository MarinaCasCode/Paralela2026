#include "escalaGrises.hpp"
#include <stdexcept> // std::runtime_error

std::vector<uint8_t> convertirEscalaGrises(const Imagen &imagen) {
    if (imagen.ancho <= 0 || imagen.alto <= 0 || imagen.canales <= 0) {
        throw std::runtime_error("Imagen inválida: dimensiones o canales no válidos");
    }

    std::vector<uint8_t> gris(imagen.ancho * imagen.alto); // vector para la imagen en escala de grises

    if (imagen.canales == 1) {
        // La imagen ya está en escala de grises, copiar directamente
        gris = imagen.datos;
    } else {
        // Convertir a escala de grises usando la fórmula de luminancia
        for (int i = 0; i < imagen.ancho * imagen.alto; ++i) {
            int idx = i * imagen.canales;
            uint8_t r = imagen.datos[idx];
            uint8_t g = imagen.datos[idx + 1];
            uint8_t b = imagen.datos[idx + 2];
            // Ignorar el canal alfa si existe
            gris[i] = static_cast<uint8_t>(0.2126 * r + 0.7152 * g + 0.0722 * b);
        }
    }

    return gris;
}