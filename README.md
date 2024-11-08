# Proyecto de Compilación y Ejecución

Este proyecto incluye varios programas implementados en C que utilizan diferentes paradigmas de programación paralela, como MPI y OpenMP, así como una versión serial. A continuación, se detallan las instrucciones necesarias para compilar y ejecutar cada uno de los archivos.

## Tabla de Contenidos

- [Prerrequisitos](#prerrequisitos)
- [Compilación y Ejecución de Programas MPI](#compilación-y-ejecución-de-programas-mpi)
  - [SUM_MPI](#sum_mpi)
  - [MATRICES_MULTIPLICACION](#matrices_multiplicacion)
- [Compilación y Ejecución de Programas Sobel](#compilación-y-ejecución-de-programas-sobel)
  - [sobel_serial](#sobel_serial)
  - [SOBEL_OPENMP](#sobel_openmp)
  - [SOBEL_MPI](#sobel_mpi)
- [Tutorial de Instalación](#tutorial-de-instalación)

## Prerrequisitos

Antes de comenzar, asegúrate de tener instalados los siguientes compiladores y bibliotecas:

- **MPI:** Debes tener instalado `mpicc`, que forma parte de la biblioteca MPI (por ejemplo, [OpenMPI](https://www.open-mpi.org/) o [MPICH](https://www.mpich.org/)).
- **GCC:** El compilador GNU C (`gcc`) para compilar programas seriales y con OpenMP.
- **Biblioteca Matemática:** Asegúrate de tener la biblioteca matemática (`libm`) disponible, generalmente incluida en la mayoría de las distribuciones de Linux.

Para verificar si `mpicc` y `gcc` están instalados, puedes ejecutar:

```bash
mpicc --version
gcc --version
```

## Compilación de sección Análisis

### SUM_MPI

**Descripción:** Este programa realiza la suma de elementos utilizando MPI para paralelizar la operación.

**Archivo Fuente:** `sum_mpi.c`

**Compilación:**

```bash
mpicc sum_mpi.c -o SUM_MPI
```

**Ejecución:**

```bash
mpirun --hostfile /etc/hosts -np <número_de_procesos> ./SUM_MPI
```

**Nota:** Reemplaza `<número_de_procesos>` con la cantidad de procesos que deseas utilizar.

## Ejercicios prácticos

### MATRICES_MULTIPLICACION

**Descripción:** Este programa realiza la multiplicación de matrices utilizando MPI para paralelizar la operación.

**Archivo Fuente:** `matrices.c`

**Compilación:**

```bash
mpicc matrices.c -o MATRICES_MULTIPLICACION
```

**Ejecución:**

```bash
mpirun --hostfile /etc/hosts -np <número_de_procesos> ./MATRICES_MULTIPLICACION
```

**Nota:** Asegúrate de que el archivo `/etc/hosts` contenga las direcciones IP o nombres de los hosts donde se ejecutarán los procesos MPI.

### sobel_serial

**Descripción:** Implementación serial del filtro Sobel para detección de bordes en imágenes.

**Archivo Fuente:** `sobel_serial.c`

**Compilación:**

```bash
gcc -o sobel_serial sobel_serial.c -lm
```

**Ejecución:**

```bash
./sobel_serial
```

### SOBEL_OPENMP

**Descripción:** Implementación del filtro Sobel utilizando OpenMP para paralelizar la operación en múltiples hilos.

**Archivo Fuente:** `sobel_openmp.c`

**Compilación:**

```bash
gcc -o SOBEL_OPENMP sobel_openmp.c -lm -fopenmp
```

**Ejecución:**

```bash
./SOBEL_OPENMP
```

### SOBEL_MPI

**Descripción:** Implementación del filtro Sobel utilizando MPI para paralelizar la operación en múltiples procesos.

**Archivo Fuente:** `sobel_mpi.c`

**Compilación:**

```bash
mpicc -o SOBEL_MPI sobel_mpi.c -lm
```

**Ejecución:**

```bash
mpirun --hostfile /etc/hosts -np <número_de_procesos> ./SOBEL_MPI
```

## Tutorial de Instalación

Para una guía detallada sobre cómo instalar y configurar el entorno para estos programas, puedes consultar este [playlist en YouTube](https://youtube.com/playlist?list=PLOB8_oGJl40Sxjn9rtgSVgg9tfC4Z4MWe&si=Lm7TKEw4iC5zTaGw), que proporciona instrucciones paso a paso para instalar las herramientas necesarias y trabajar con MPI, OpenMP y compilación en C.
