
# Tarea 4: Procesamiento de Imágenes en GPU con OpenACC
 
**CI-0117 Programación Paralela y Concurrente — UCR, I Ciclo 2026**
 
Pipeline de procesamiento de imágenes que lee una imagen, la convierte a escala de
grises, le aplica un filtro Gaussiano (Gaussian Blur) mediante convolución, y guarda
los resultados en disco. La etapa del **blur Gaussiano** está acelerada con **OpenACC**
(`#pragma acc data`, `#pragma acc parallel loop collapse(2)`), y se compara su tiempo
de ejecución contra una versión equivalente en CPU para calcular el speedup.
 
## Estructura
 
```
Tarea3_120726/
├── Makefile                    # compila las versiones CPU (g++) y GPU (nvc++)
├── README.md
├── include/
│   ├── imagen_io.hpp           # struct Imagen + carga/guardado de archivos
│   ├── escalaGrises.hpp        # conversión RGB -> escala de grises
│   ├── gaussianBlur.hpp        # blur Gaussiano 3x3 en CPU
│   ├── gaussianBlurGPU.hpp     # blur Gaussiano 3x3 en GPU (OpenACC)
│   ├── stb_image.h             # librería de lectura de imágenes (third-party)
│   └── stb_image_write.h       # librería de escritura de imágenes (third-party)
├── src/
│   ├── main.cpp                # pipeline principal + menú de selección de imagen
│   ├── imagen_io.cpp
│   ├── escalaGrises.cpp
│   ├── gaussianBlur.cpp        # implementación CPU
│   └── gaussianBlurGPU.cpp     # implementación GPU (OpenACC)
├── imagenesEntrada/            # imágenes de entrada (.png, .jpg, .jpeg, .bmp, .tga)
└── imagenesSalida/             # imágenes generadas por el programa
```
 
## Algoritmo
 
### 1. Conversión a escala de grises
 
Cada píxel RGB se convierte a un solo canal de gris usando la fórmula estándar de
luminancia, que aproxima cómo el ojo humano percibe el brillo de cada color:
 
```
Gris = 0.299 × R + 0.587 × G + 0.114 × B
```
 
Esta etapa corre en CPU y no requiere aceleración GPU según el enunciado de la tarea.
 
### 2. Blur Gaussiano (convolución 3×3)
 
Se usa el kernel Gaussiano 3×3 clásico, normalizado dividiendo entre 16:
 
```
1 2 1
2 4 2      / 16
1 2 1
```
 
Para cada píxel `(y, x)`, el valor filtrado se calcula como el promedio ponderado de
sus 8 vecinos y de sí mismo:
 
```
B(y,x) = Σ Σ K(ky,kx) * I(y+ky, x+kx)      ky,kx ∈ [-1, 1]
```
 
**Manejo de bordes:** se usa la técnica de *clamp*, es decir, cuando el kernel se sale
de los límites de la imagen se reutiliza el píxel válido más cercano:
 
```
yy = min(max(y+ky, 0), H-1)
xx = min(max(x+kx, 0), W-1)
```
 
**Nota sobre intensidad del blur:** un solo pase de un kernel 3×3 es sutil en imágenes
de alta resolución con mucho detalle. Por eso el kernel se aplica en cascada
`numIteraciones` veces (60 por defecto, definido en `main.cpp`), retroalimentando el
resultado de cada pasada como entrada de la siguiente. El mismo número de pasadas se
usa tanto en CPU como en GPU para que la comparación de tiempos sea justa.
 
## Compilación
 
Requiere un compilador con soporte de C++17. La versión GPU además requiere el
**NVIDIA HPC SDK** (`nvc++`) con soporte OpenACC.
 
```bash
make            # compila la version CPU con g++ (ignora los pragmas acc)
make gpu        # compila la version GPU con nvc++ (requiere NVIDIA HPC SDK)
make clean      # elimina los ejecutables generados
```
 
> En una máquina sin GPU NVIDIA (por ejemplo una Mac), `make` compila igual porque
> `g++` simplemente ignora las directivas `#pragma acc`, ejecutando el código de forma
> secuencial. Esto sirve para validar que el algoritmo y las imágenes de salida sean
> correctas antes de medir el rendimiento real en GPU. El tiempo "GPU" medido así
> **no es representativo** — el rendimiento real solo se mide en Kabré (ver abajo).
 
## Ejecución
 
El programa incluye un **menú interactivo**: al ejecutarlo sin argumentos, escanea la
carpeta `imagenesEntrada/` y lista todas las imágenes disponibles (`.png`, `.jpg`,
`.jpeg`, `.bmp`, `.tga`) para que el usuario elija cuál procesar.
 
```bash
make run        # compila si hace falta y ejecuta con menu (version CPU/local)
make run-gpu    # compila si hace falta y ejecuta con menu (version GPU, en Kabre)
```
 
Ejemplo de menú:
 
```
============================================
 SELECCION DE IMAGEN DE ENTRADA
============================================
  [1] gatito.jpg
  [2] perro.png
  [3] paisaje.jpg
  [4] flor.png
  [5] auto.jpg
 
Elige el numero de la imagen a procesar:
```
 
También se puede indicar la ruta de la imagen directamente como argumento, sin pasar
por el menú (útil para pruebas rápidas o scripts):
 
```bash
./imagenesSalida/test_blur imagenesEntrada/gatito.jpg
```
 
Al finalizar, el programa:
1. Imprime en terminal los tiempos de CPU, GPU y el speedup (`T_CPU / T_GPU`).
2. Guarda en `imagenesSalida/` la imagen original en escala de grises y las dos
   versiones con blur (CPU y GPU), nombradas según la imagen de entrada
   (ej. `gatito_gris.png`, `gatito_blurCPU.png`, `gatito_blurGPU.png`).
3. Abre automáticamente las 4 imágenes (original, gris, blur CPU, blur GPU) con el
   visor de imágenes por defecto del sistema operativo, para comparación visual
   inmediata. En sistemas sin entorno gráfico (como los nodos de cómputo de Kabré),
   este paso simplemente no tiene efecto y no detiene la ejecución.
## Ejecución en Kabré (nodo nukwa)
 
Los tiempos de CPU y GPU que van en el informe deben medirse **en Kabré, en el mismo
nodo**, para que la comparación y el speedup sean válidos.
 
### 1. Ingresar a Kabré
 
```bash
ssh usuario@kabre.cenat.ac.cr
```
 
### 2. Subir el proyecto
 
Desde la máquina local, con el proyecto ya probado y funcionando:
 
```bash
scp -r Tarea3_120726 usuario@kabre.cenat.ac.cr:~/
```
 
### 3. Cargar el módulo del NVIDIA HPC SDK
 
El nombre exacto del módulo puede variar; verificarlo con:
 
```bash
module avail
module avail nvhpc
```
 
Una vez identificado el módulo correcto, cargarlo (ejemplo genérico):
 
```bash
module load nvhpc
```
 
### 4. Solicitar un nodo con GPU (nukwa) vía Slurm
 
Kabré usa Slurm para gestionar la cola de trabajos. Verificar con el curso o con
`sinfo` el nombre exacto de la partición del nodo nukwa antes de enviar el trabajo,
por ejemplo:
 
```bash
srun --partition=nukwa --gres=gpu:1 --pty bash
```
 
### 5. Compilar y ejecutar la versión GPU
 
```bash
cd Tarea3_120726
make clean
make gpu
make run-gpu
```
 
`make gpu` compila con `nvc++ -acc -Minfo=accel`; la bandera `-Minfo=accel` muestra en
consola qué bucles fueron efectivamente paralelizados en GPU, útil para confirmar que
las directivas OpenACC se aplicaron correctamente.
 
### 6. Recolectar resultados para el informe
 
Del mismo nodo, correr también la versión CPU para tener ambos tiempos en igualdad de
condiciones de hardware:
 
```bash
make run
```
 
Anotar de la salida en terminal:
- Tiempo CPU (ms)
- Tiempo GPU (ms)
- Speedup (`T_CPU / T_GPU`)
Y copiar las imágenes generadas en `imagenesSalida/` de vuelta a la máquina local para
incluirlas en el informe:
 
```bash
scp -r usuario@kabre.cenat.ac.cr:~/Tarea3_120726/imagenesSalida ./imagenesSalida_kabre
```
 
## Directivas OpenACC utilizadas
 
| Directiva | Uso en el proyecto |
|---|---|
| `#pragma acc data copyin(...) copyout(...)` | Transfiere la imagen de entrada a la GPU una sola vez por llamada, y trae de vuelta el resultado a CPU al finalizar. |
| `#pragma acc parallel loop collapse(2)` | Paraleliza los bucles anidados en `y` y `x`, tratándolos como un único espacio de iteración repartido entre los hilos de la GPU. |
 
Implementación completa en `src/gaussianBlurGPU.cpp`.