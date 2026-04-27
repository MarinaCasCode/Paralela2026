#!/bin/bash
#SBATCH --job-name=dining_philosophers
#SBATCH --output=dining_philosophers_%j.out
#SBATCH --error=dining_philosophers_%j.err
#SBATCH --ntasks=1
#SBATCH --cpus-per-task=20
#SBATCH --mem=512M
#SBATCH --time=00:02:00
#SBATCH --partition=kura

# Compilar
make

if [ $? -ne 0 ]; then
    echo "Error de compilacion"
    exit 1
fi

echo "Nucleos disponibles: $(nproc)"
echo "Iniciando simulacion..."

# Ejecutar con timeout para evitar loop infinito
timeout 60s ./diningPhilosophers

echo "Simulacion finalizada"