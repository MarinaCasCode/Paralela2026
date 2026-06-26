#!/bin/bash
#SBATCH --job-name=cenatMD_viz
#SBATCH --partition=kura-debug
#SBATCH --nodes=1
#SBATCH --ntasks=8
#SBATCH --cpus-per-task=2
#SBATCH --time=00:15:00
#SBATCH --output=cenatMD_viz_%j.out
#SBATCH --error=cenatMD_viz_%j.err

# ============================================================
# Tarea 2 - cenatMD - Generacion de frames para la visualizacion
#   Colision de dos cumulos (scenario=1), 1600 particulas, 2000 iter.
#   Escribe posiciones_*.csv cada 100 iter -> ~20 frames para Paraview.
# ============================================================
set -e

module load openmpi
module load mpfr/3.1.5

echo ">>> Compilando..."
make clean && make

export OMP_NUM_THREADS=$SLURM_CPUS_PER_TASK

rm -f posiciones_*.csv
echo ">>> Generando frames de la colision (np 8)..."
time mpiexec --bind-to none -np $SLURM_NTASKS ./cenatMD 200 2000 1 0 1

echo ">>> Frames generados:"
ls posiciones_*.csv | wc -l