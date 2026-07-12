# Makefile para Tarea 4 - Procesamiento de Imagenes en GPU
# Uso:
#   make            -> compila el programa
#   make run         -> compila y ejecuta con imagenesEntrada/gatito.jpg
#   make clean       -> borra el ejecutable generado

CXX = g++
CXXFLAGS = -std=c++17 -O2 -I include

SRC = src/main.cpp src/imagen_io.cpp src/escalaGrises.cpp src/gaussianBlur.cpp
OUT = imagenesSalida/test_blur

IMG = imagenesEntrada/gatito.jpg

.PHONY: all run clean

all: $(OUT)

$(OUT): $(SRC)
	$(CXX) $(CXXFLAGS) $(SRC) -o $(OUT)

run: $(OUT)
	./$(OUT) $(IMG)

clean:
	rm -f $(OUT)