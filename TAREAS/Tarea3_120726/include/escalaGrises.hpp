// Convertir imagen a escala de grises 

#ifndef ESCALA_GRIS_HPP
#define ESCALA_GRIS_HPP

#include "imagen_io.hpp"
#include <cstdint>
#include <vector>

// COnvertir una imagen a escala de grises en base a la formula de luminancia *Explicado en README
// gris = 0.2126 * R + 0.7152 * G + 0.0722 * B
// Si la imagen ya tiene un solo canal (gris), se copia tal cual
// Si no, se ignora el canal alfa (si lo tiene) y se aplica la formula de luminancia a cada pixel
// devuelve un vector de tam ancho*alto (cada pixel es un byte) 
std::vector<uint8_t> convertirEscalaGrises(const Imagen &imagen);

#endif // ESCALA_GRIS_HPP
