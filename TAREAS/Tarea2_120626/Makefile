# Makefile - Tarea 2 MPI+OpenMP (cenatMD)
#
# Uso:
#   make         -> compila el binario ./cenatMD
#   make clean   -> elimina el binario
#
# Requiere un compilador MPI (mpicxx) con soporte OpenMP.
# En Kabre, cargar primero el modulo de MPI (ver README):
#   module load openmpi

MPICXX   = mpicxx
CXXFLAGS = -O2 -fopenmp -Wall -Wextra -std=c++17
TARGET   = cenatMD
SRCDIR   = src
SRC      = $(SRCDIR)/cenatMD.cpp

$(TARGET): $(SRC)
	$(MPICXX) $(CXXFLAGS) -o $(TARGET) $(SRC)

clean:
	rm -f $(TARGET)

.PHONY: clean