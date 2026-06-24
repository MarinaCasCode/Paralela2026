#!/bin/bash
#SBATCH --job-name=cenatMD_perf
#SBATCH --partition=kura
#SBATCH --nodes=1
#SBATCH --ntasks=16
#SBATCH --ntasks-per-node=16
#SBATCH --cpus-per-task=2
#SBATCH --time=00:20:00
#SBATCH --output=cenatMD_perf_%j.out
#SBATCH --error=cenatMD_perf_%j.err

# ============================================================
# Corrida de desempeno (competencia)
#   mpiexec -np 16 ./cenatMD 200 7000 0 0
# Config hibrida: 1 nodo x 16 ranks x 2 hilos = 32 cores.
# Binding: lo maneja SLURM (cpus-per-task=2); mpiexec con --bind-to none
# ============================================================
set -e

module load openmpi
module load mpfr/3.1.5

echo ">>> Compilando en nodo de computo..."
make clean && make

export OMP_NUM_THREADS=$SLURM_CPUS_PER_TASK

echo "============================================================"
echo "Nodos:  $SLURM_JOB_NODELIST"
echo "Ranks:  $SLURM_NTASKS  |  Hilos/rank: $OMP_NUM_THREADS"
echo "============================================================"

echo ">>> Corrida de desempeno (np 16, N=200, 7000 iter)..."
mpiexec --bind-to none -np $SLURM_NTASKS ./cenatMD 200 7000 0 0

echo ""
echo "============================================================"
echo "Corrida completada"
echo "============================================================"