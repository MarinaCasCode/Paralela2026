#!/bin/bash
#SBATCH --job-name=cenatMD_verif
#SBATCH --partition=kura-debug
#SBATCH --nodes=1
#SBATCH --ntasks-per-node=8
#SBATCH --cpus-per-task=5
#SBATCH --time=00:05:00
#SBATCH --output=cenatMD_verif_%j.out
#SBATCH --error=cenatMD_verif_%j.err

# ============================================================
# Tarea 2 - cenatMD - Verificacion MPI+OpenMP en Kura (Kabre)
# ============================================================
# Lanza la corrida de validacion del enunciado: np 8, 100 100 1 1.
# Dimensionamiento (kura-debug = limite 1 nodo, 40 cores/nodo):
#   1 nodo x 8 ranks = 8 ranks MPI
#   5 hilos OpenMP por rank  ->  8*5 = 40 cores (nodo lleno)
# ============================================================

set -e

echo "============================================================"
echo "cenatMD - Verificacion en Kura"
echo "Fecha:      $(date)"
echo "Nodos:      $SLURM_JOB_NODELIST"
echo "Ranks MPI:  $SLURM_NTASKS"
echo "Hilos/rank: $SLURM_CPUS_PER_TASK"
echo "============================================================"

# Modulos: openmpi + mpfr (gcc 9.3.0 necesita libmpfr.so.4 para compilar)
module load openmpi
module load mpfr/3.1.5

echo ""
echo ">>> Compilando..."
make clean || true
make

# Un hilo OpenMP por core asignado a cada rank
export OMP_NUM_THREADS=$SLURM_CPUS_PER_TASK

echo ""
echo ">>> Ejecutando validacion (np 8)..."
mpiexec -np $SLURM_NTASKS ./cenatMD 100 100 1 1

echo ""
echo "============================================================"
echo "Verificacion completada"
echo "============================================================"