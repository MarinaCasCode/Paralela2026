#!/bin/bash
#SBATCH --job-name=cenatMD_verif
#SBATCH --partition=kura-debug
#SBATCH --nodes=2
#SBATCH --ntasks-per-node=2
#SBATCH --cpus-per-task=20
#SBATCH --time=00:05:00
#SBATCH --output=cenatMD_verif_%j.out
#SBATCH --error=cenatMD_verif_%j.err

# ============================================================
# Tarea 2 - cenatMD - Verificacion MPI+OpenMP en Kura (Kabre)
# ============================================================
# Fase 0: solo confirma que el binario hibrido corre y que cada
# rank MPI ve sus hilos OpenMP. NO mide tiempos todavia.
#
# Dimensionamiento (kura = 40 cores/nodo):
#   2 nodos x 2 ranks/nodo = 4 ranks MPI
#   20 hilos OpenMP por rank  ->  2*20 = 40 cores por nodo (nodo lleno)
#
# ============================================================

set -e  # parar si algo falla

echo "============================================================"
echo "cenatMD - Verificacion en Kura"
echo "Fecha:        $(date)"
echo "Nodos:        $SLURM_JOB_NODELIST"
echo "Ranks MPI:    $SLURM_NTASKS"
echo "Hilos/rank:   $SLURM_CPUS_PER_TASK"
echo "============================================================"

# Cargar el modulo de MPI (verificar nombre exacto con: module avail)
module load openmpi

# Compilar
echo ""
echo ">>> Compilando..."
make clean || true
make

# Un hilo OpenMP por core asignado a cada rank
export OMP_NUM_THREADS=$SLURM_CPUS_PER_TASK

# Ejecutar la verificacion Fase 0 (N e ITERACIONES aun no se usan)
echo ""
echo ">>> Ejecutando cenatMD..."
mpiexec -np $SLURM_NTASKS ./cenatMD 100 100 0 1

echo ""
echo "============================================================"
echo "Verificacion completada"
echo "============================================================"