#!/bin/bash
#SBATCH --job-name=ejercicio2
#SBATCH --output=resultado_ejercicio2.txt
#SBATCH --partition=kura
#SBATCH --ntasks=1
#SBATCH --time=00:10:00

nproc
gcc -std=c99 -o ejercicio2 ejercicio2.1.c -lpthread
srun ./ejercicio2