# Makefile - Tarea 2 Heat3D
# Para compilar secuencial: make (con '#define USE_OPENMP' en heat3d.cpp comentado)
# Para compilar con OpenMP: make (con '#define USE_OPENMP' en heat3d.cpp activo)

CXX      = g++
CXXFLAGS = -O2 -fopenmp -Wall
TARGET   = heat3d
SRC      = heat3d.cpp

$(TARGET): $(SRC)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(SRC)

clean:
	rm -f $(TARGET)

.PHONY: clean

