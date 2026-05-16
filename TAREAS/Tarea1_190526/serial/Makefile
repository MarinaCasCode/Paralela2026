#Makefile - Tarea 1 Heat3D Serial
# Para compilar: make

CXX = g++
CXXFLAGS = -O2 -Wall
TARGET = heat3dSerial
SRC = heat3dSerial.cpp

$(TARGET): $(SRC)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(SRC)

clean:
	rm -f $(TARGET)

.PHONY: clean
