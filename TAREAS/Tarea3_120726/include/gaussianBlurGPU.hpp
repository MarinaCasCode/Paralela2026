#ifndef GAUSSIAN_BLUR_GPU_HPP
#define GAUSSIAN_BLUR_GPU_HPP

#include <vector>
#include <cstdint>

// Mismo kernel gaussiano 3x3 normalizado que en CPU, pero aplicado en GPU

std::vector<uint8_t> gaussianBlurGPU(const std::vector<uint8_t>& imagen, int ancho, int alto, int numPasadas);

#endif // GAUSSIAN_BLUR_GPU_HPP