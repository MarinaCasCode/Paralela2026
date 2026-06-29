
#!/bin/bash
#SBATCH --job-name=stencil1d         # nombre del trabajo
#SBATCH --output=salida_%j.txt       # archivo de salida (%j = ID del job)
#SBATCH --error=error_%j.txt         # archivo de errores
#SBATCH --ntasks=4                   # número de procesos MPI
#SBATCH --time=00:05:00              # tiempo máximo de ejecución (5 minutos)
#SBATCH --partition=normal           # partición/cola del cluster
 
# cargar el módulo de MPI disponible en el Kabré
module load mpi/openmpi
 
# compilar el programa
mpicc -O2 -o stencil1d main.c
 
# ejecutar con 4 procesos, N=16 elementos, M=10 iteraciones
mpirun -np 4 ./stencil1d 16 10
 