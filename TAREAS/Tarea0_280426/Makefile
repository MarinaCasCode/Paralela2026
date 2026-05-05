# Makefile - Tarea 0: Simulacion de check-in de aeropuerto

# ============================================================
# Variables
# ============================================================
CC      := gcc
CFLAGS  := -std=c99 -D_DEFAULT_SOURCE -Wall -Wextra -O2 -pthread
LDFLAGS := -pthread

# Codigo fuente comun a todos los binarios (sin main)
SRC_LIB := counter.c pasajero.c queue.c supervisor.c balancer.c

# Binarios
BIN_MAIN := airport
BIN_TEST := testCounter

# Parametros del modo competencia (segun el enunciado)
#   N=3000  M_eco=4  M_biz=2  M_intl=2  K_min=3  K_max=8  T_max=500  Q=50
COMP_ARGS := 3000 4 2 2 3 8 500 50

# Parametros para test rapido (N pequeno)
TEST_ARGS := 100 4 2 2 3 8 500 50

# ============================================================
# Targets
# ============================================================

# Default: compila el binario principal
all: $(BIN_MAIN)

# Compilacion del binario principal
$(BIN_MAIN): main.c $(SRC_LIB)
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

# Compilacion del test del counter (sin balancer ni main)
$(BIN_TEST): testCounter.c counter.c pasajero.c queue.c supervisor.c
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

# Test rapido con parametros chicos
test: $(BIN_MAIN)
	./$(BIN_MAIN) $(TEST_ARGS)

# Modo competencia oficial
competencia: $(BIN_MAIN)
	./$(BIN_MAIN) $(COMP_ARGS)

# Verificacion de memoria con Valgrind (debe reportar 0 bytes lost)
valgrind: $(BIN_MAIN)
	valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes \
	         ./$(BIN_MAIN) $(TEST_ARGS)

# Verificacion de data races / deadlocks con Helgrind
helgrind: $(BIN_MAIN)
	valgrind --tool=helgrind --history-level=approx \
	         ./$(BIN_MAIN) $(TEST_ARGS)

# Verificacion alterna con DRD
drd: $(BIN_MAIN)
	valgrind --tool=drd ./$(BIN_MAIN) $(TEST_ARGS)

# Limpieza de binarios y temporales
clean:
	rm -f $(BIN_MAIN) $(BIN_TEST) *.o

# Indicar que estos targets no son archivos
.PHONY: all test competencia valgrind helgrind drd clean