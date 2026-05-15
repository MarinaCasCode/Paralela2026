# Makefile para Shear Sort
# Targets:
#   make            -> versión paralela (con OpenMP)
#   make seq        -> versión secuencial (sin OpenMP)
#   make clean      -> elimina ejecutables y archivos temporales

CXX       = g++
CXXFLAGS  = -O2 -Wall -Wextra -std=c++17
OMPFLAGS  = -fopenmp

SRC       = shear.cpp
BIN       = shear
BIN_SEQ   = shear_seq

.PHONY: all par seq clean

all: par

par: $(SRC)
	$(CXX) $(CXXFLAGS) $(OMPFLAGS) -o $(BIN) $(SRC)

seq: $(SRC)
	$(CXX) $(CXXFLAGS) -o $(BIN_SEQ) $(SRC)

clean:
	rm -f $(BIN) $(BIN_SEQ) *.o