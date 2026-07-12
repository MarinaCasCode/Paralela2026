# Makefile para Tarea 4 - Procesamiento de Imagenes en GPU
#
# Uso local (Mac/Linux sin GPU, solo valida CPU y correctitud del codigo):
#   make            -> compila con g++ (ignora los #pragma acc)
#   make run        -> compila y ejecuta, mostrando el menu de seleccion de imagen
#
# Uso en Kabre (nodo nukwa, con NVIDIA HPC SDK):
#   make gpu        -> compila con nvc++ y soporte real de OpenACC
#   make run-gpu    -> compila con nvc++ y ejecuta, mostrando el menu de seleccion de imagen
#
#   make clean      -> borra los ejecutables generados

SRC = src/main.cpp src/imagen_io.cpp src/escalaGrises.cpp src/gaussianBlur.cpp src/gaussianBlurGPU.cpp

# ---- Compilacion local (g++, sin GPU real) ----
CXX = g++
CXXFLAGS = -std=c++17 -O2 -I include
OUT = imagenesSalida/test_blur

# ---- Compilacion en Kabre (nvc++, con OpenACC real) ----
CXX_ACC = nvc++
CXXFLAGS_ACC = -std=c++17 -O2 -I include -acc -Minfo=accel
OUT_ACC = imagenesSalida/test_blur_gpu

.PHONY: all run gpu run-gpu clean

all: $(OUT)

$(OUT): $(SRC)
	$(CXX) $(CXXFLAGS) $(SRC) -o $(OUT)

run: $(OUT)
	./$(OUT)

gpu: $(SRC)
	$(CXX_ACC) $(CXXFLAGS_ACC) $(SRC) -o $(OUT_ACC)

run-gpu: gpu
	./$(OUT_ACC)

clean:
	rm -f $(OUT) $(OUT_ACC)