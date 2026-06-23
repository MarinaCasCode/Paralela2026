# Tarea 2: Simulación n-body con MPI + OpenMP (`cenatMD`)

**CI-0117 Programación Paralela y Concurrente UCR, I Ciclo 2026**

Simulación de interacción de partículas (fuerzas de Van der Waals) en un sistema tridimensional,
paralelizada con **MPI** (memoria distribuida, topología de anillo) y **OpenMP**
(paralelismo de hilos en el cálculo de fuerzas).

## Estructura

```
Tarea2_OpenMPI/
├── Makefile          # compila el binario cenatMD
├── src/
│   └── cenatMD.cpp   # programa principal
├── mediciones/       # scripts SLURM y resultados de desempeño
└── viz/              # datasets y archivos de visualización (Paraview)
```

## Compilación

Requiere un compilador MPI (`mpicxx`) con soporte OpenMP.

```bash
make          # genera ./cenatMD
make clean    # elimina el binario
```

En el cluster Kabré, cargar primero el módulo de MPI (verificar el nombre exacto
con `module avail`):

```bash
module load openmpi
make
```

## Ejecución

```bash
mpiexec -np <#Ranks> ./cenatMD <N> <ITERACIONES> <BANDERA_IMP> <BANDERA_INI>
```

| Argumento      | Significado |
| -------------- | ----------- |
| `N`            | cantidad de partículas **por proceso** |
| `ITERACIONES`  | número de iteraciones de la simulación |
| `BANDERA_IMP`  | `1` = escribir posiciones cada 100 iteraciones |
| `BANDERA_INI`  | `1` = posiciones fijas (validación), `0` = aleatorias |

Ejemplos:

```bash
mpiexec -np 8  ./cenatMD 100 100  1 1   # validación
mpiexec -np 16 ./cenatMD 200 7000 0 0   # desempeño
```
