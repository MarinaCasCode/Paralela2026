# Makefile - Tarea 2 Heat3D

CXX      = g++
CXXFLAGS = -O2 -fopenmp -Wall
TARGET   = heat3d
SRC      = heat3d.cpp

$(TARGET): $(SRC)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(SRC)

clean:
	rm -f $(TARGET)

.PHONY: clean