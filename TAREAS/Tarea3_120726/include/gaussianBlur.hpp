#ifndef GAUSSUAN_BLUR_HPP
#define GAUSSUAN_BLUR_HPP

#include <vector>
#include <cstdint> // uint8_t, uint32_t, para tipos de datos enteros de tamaño fijo

// Aplicar filtro gaussiano 3x3 a una imagen en escala de grises en CPU
// entrada: vector de bytes (uint8_t) de tamaño ancho*alto, cada pixel es un byte (0-255)
// devuelve: nuevo vector de bytes (uint8_t) de tamaño ancho*alto, cada pixel es un byte (0-255), no se modifica la entrada
// kernel usado: dividido entre 16 (para normalizar)
// 1 2 1
// 2 4 2
// 1 2 1
// Manejo de bordes: se utiliiza clamp (usar el pixel válido más cercano) para los bordes de laimagen yq ue no queden en negro

std::vector<uint8_t>gaussianBlurCPU(const std::vector<uint8_t> &imagen, int ancho, int alto);

#endif // GAUSSUAN_BLUR_HPP