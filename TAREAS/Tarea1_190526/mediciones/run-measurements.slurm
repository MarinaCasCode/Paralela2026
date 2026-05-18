#!/bin/bash
#SBATCH --job-name=heat3d_meas
#SBATCH --partition=kura
#SBATCH --nodes=1
#SBATCH --ntasks=1
#SBATCH --cpus-per-task=16
#SBATCH --time=01:00:00
#SBATCH --output=heat3d_meas_%j.out
#SBATCH --error=heat3d_meas_%j.err

# ============================================================
# Tarea 1 - Heat3D - Mediciones de tiempo en Kura (Kabré)
# Strong scaling + Weak scaling
# ============================================================
# Este script asume la siguiente estructura de carpetas:
#   Tarea1_190526/
#     ├── serial/          (heat3dSerial.cpp + Makefile)
#     ├── paralelo/        (heat3d.cpp + Makefile)
#     └── mediciones/      (este script y resultados)
# Lanzarlo desde la carpeta mediciones/:
#   sbatch run_measurements.slurm
# ============================================================

set -e  # parar si algo falla

# Carpeta raíz de la tarea (un nivel arriba de mediciones/)
ROOT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
SERIAL_DIR="$ROOT_DIR/serial"
PARALELO_DIR="$ROOT_DIR/paralelo"
RESULTS_DIR="$(cd "$(dirname "$0")" && pwd)"

# Archivo CSV de salida
CSV="$RESULTS_DIR/results.csv"

# Limpiar/crear el CSV con encabezado
echo "version,N,threads,rep,time_seconds" > "$CSV"

echo "============================================================"
echo "Heat3D - Mediciones en Kura"
echo "Fecha: $(date)"
echo "Nodo:  $(hostname)"
echo "Cores asignados (cpus-per-task): $SLURM_CPUS_PER_TASK"
echo "============================================================"

# ============================================================
# Compilación
# ============================================================
echo ""
echo ">>> Compilando versión secuencial..."
cd "$SERIAL_DIR"
make clean || true
make
SERIAL_BIN="$SERIAL_DIR/heat3dSerial"

echo ""
echo ">>> Compilando versión paralela..."
cd "$PARALELO_DIR"
make clean || true
make
PAR_BIN="$PARALELO_DIR/heat3d"

# ============================================================
# Función auxiliar: extrae el tiempo del stdout del programa
# El programa imprime: "Simulación terminada en X.XXXX segundos"
# ============================================================
extract_time() {
    grep -oP 'Simulaci[oó]n terminada en \K[0-9.]+' "$1" | head -1
}

# Configuración: 5 repeticiones por cada combinación
REPS=5
NUM_STEPS=1000

# ============================================================
# STRONG SCALING
# N fijo, varía cantidad de hilos
# Para cada N: secuencial + paralelo con 1, 2, 4, 8, 16 hilos
# ============================================================
echo ""
echo "============================================================"
echo "STRONG SCALING"
echo "============================================================"

cd "$RESULTS_DIR"

for N in 100 200; do
    echo ""
    echo ">>> N = $N"

    # --- Secuencial (referencia) ---
    for rep in $(seq 1 $REPS); do
        echo "  [serial]   N=$N rep=$rep"
        OUT=$("$SERIAL_BIN" $N $NUM_STEPS)
        T=$(echo "$OUT" | grep -oP 'Simulaci[oó]n terminada en \K[0-9.]+' | head -1)
        echo "serial,$N,1,$rep,$T" >> "$CSV"
        echo "    tiempo = $T s"
    done

    # --- Paralelo con T hilos ---
    for T_THREADS in 1 2 4 8 16; do
        for rep in $(seq 1 $REPS); do
            echo "  [paralelo] N=$N threads=$T_THREADS rep=$rep"
            OUT=$(OMP_NUM_THREADS=$T_THREADS "$PAR_BIN" $N $NUM_STEPS)
            T=$(echo "$OUT" | grep -oP 'Simulaci[oó]n terminada en \K[0-9.]+' | head -1)
            echo "parallel,$N,$T_THREADS,$rep,$T" >> "$CSV"
            echo "    tiempo = $T s"
        done
    done
done

# ============================================================
# WEAK SCALING
# N escala con hilos^(1/3) para mantener trabajo/hilo constante
# Base: N=100 con 1 hilo (1.0M puntos)
# ============================================================
echo ""
echo "============================================================"
echo "WEAK SCALING (trabajo/hilo constante)"
echo "============================================================"

# Pares (hilos, N) tales que N^3/hilos ≈ 100^3
declare -a WEAK_THREADS=(1 2 4 8 16)
declare -a WEAK_N=(100 126 159 200 252)

for i in 0 1 2 3 4; do
    T_THREADS=${WEAK_THREADS[$i]}
    N=${WEAK_N[$i]}
    for rep in $(seq 1 $REPS); do
        echo "  [weak]     threads=$T_THREADS N=$N rep=$rep"
        OUT=$(OMP_NUM_THREADS=$T_THREADS "$PAR_BIN" $N $NUM_STEPS)
        T=$(echo "$OUT" | grep -oP 'Simulaci[oó]n terminada en \K[0-9.]+' | head -1)
        echo "weak,$N,$T_THREADS,$rep,$T" >> "$CSV"
        echo "    tiempo = $T s"
    done
done

echo ""
echo "============================================================"
echo "Mediciones completadas"
echo "Resultados guardados en: $CSV"
echo "============================================================"
