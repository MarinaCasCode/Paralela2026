#!/bin/bash
#SBATCH --job-name=strassen
#SBATCH --output=strassen_%j.out
#SBATCH --error=strassen_%j.err
#SBATCH --nodes=1
#SBATCH --ntasks=1
#SBATCH --cpus-per-task=16
#SBATCH --time=00:05:00
#SBATCH --partition=kura

g++ strassen.cpp -o strassen_seq -std=c++17
g++ strassen.cpp -o strassen_omp -fopenmp -std=c++17

echo "=== Secuencial ==="
./strassen_seq

echo ""
echo "=== Paralelo ==="
for THREADS in 1 2 4 7 8 16; do
    echo -n "OMP_NUM_THREADS=$THREADS -> "
    OMP_NUM_THREADS=$THREADS ./strassen_omp
done
