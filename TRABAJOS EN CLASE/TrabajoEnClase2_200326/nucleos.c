// Función para obtener el número de núcleos disponibles en el sistema
// compilacion: gcc -o nucleos nucleos.c
// Jimena Bejarano Sanchez C31074
// Marina Castro Peralta C31886

#include <stdio.h>
#include <windows.h>

int main() {
    SYSTEM_INFO info;
    GetSystemInfo(&info);
    printf("Nucleos disponibles: %d\n", info.dwNumberOfProcessors);
    return 0;
}

// Observamos que el número de núcleos disponibles en el sistema es 20, lo que significa que el sistema puede ejecutar hasta 20 threads simultáneamente sin necesidad de compartir recursos.