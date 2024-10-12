# Proyecto de Búsqueda de Llaves en DES usando MPI y OpenMP

Este proyecto implementa la búsqueda de una clave privada para descifrar un mensaje cifrado con el algoritmo DES (Data Encryption Standard) utilizando técnicas de paralelización como MPI (Message Passing Interface) y OpenMP.

## Descripción

El proyecto incluye cuatro versiones diferentes de la implementación:

1. **Naive**: Divide el rango de búsqueda de claves de manera equitativa entre los procesos MPI.
2. **OMP**: Utiliza OpenMP para dividir la carga entre múltiples hilos dentro de cada proceso MPI.
3. **División en Bloques (dbloques)**: Optimiza la distribución de claves dividiendo el rango de búsqueda en bloques para mejorar el balance de carga.
4. **Secuencial**: Realiza la búsqueda de claves de manera secuencial sin usar paralelización.

## Requisitos

### Software:

1. **Compilador con soporte para MPI y OpenMP**:
   - `mpicc` para MPI.
   - `gcc` o `clang` para OpenMP.

2. **OpenSSL**: Necesario para cifrar y descifrar mensajes usando DES.
   - En macOS:
     ```bash
     brew install openssl libomp
     ```
   - En Ubuntu:
     ```bash
     sudo apt-get install libssl-dev libomp-dev
     ```

### Hardware:

- Se recomienda ejecutar este proyecto en sistemas con múltiples núcleos o en clústeres para aprovechar la paralelización.

## Instalación

1. Clona el repositorio en tu máquina local:
   ```bash
   git clone https://github.com/mvrcentes/Computacion_Paralela_2024/tree/proyecto-2
   cd Computacion_Paralela_2024/proyecto-2
   ```

2. Asegúrate de tener instaladas las librerías requeridas, como OpenSSL y OpenMP.

## Compilación

Este proyecto usa un archivo `Makefile` para facilitar la compilación de todas las versiones.

Para compilar todas las versiones (naive, omp, dbloques y secuencial):

```bash
make
```

Esto generará los ejecutables para las diferentes implementaciones.

### Compilar manualmente

Si prefieres compilar una versión en particular manualmente, por ejemplo la versión `naive`:

```bash
mpicc -Wall -O2 -Xpreprocessor -fopenmp -I/opt/homebrew/opt/openssl@3/include -L/opt/homebrew/opt/openssl@3/lib naive.c -o naive -lcrypto -lomp -Wno-deprecated-declarations -Wunused-variable -Wunused-but-set-variable
```

## Ejecución

### Ejecución con MPI y OpenMP

Puedes ejecutar las diferentes versiones paralelas usando el comando `mpirun` para MPI. A continuación se presentan ejemplos de ejecución para cada versión:

#### 1. Naive (con MPI)

```bash
make run_alt1
```

#### 2. Naive con OpenMP (MPI + OpenMP)

```bash
make run_alt2
```

#### 3. División en Bloques (MPI + OpenMP)

```bash
make run_div
```

#### 4. Secuencial (sin MPI)

```bash
make run_seq
```

### Parámetros de Ejecución

El programa toma los siguientes parámetros:

1. **Archivo de texto**: Nombre del archivo que contiene el texto cifrado (por ejemplo, `medium.txt`).
2. **Clave inicial**: La clave usada para el cifrado del texto original.
3. **Cadena de búsqueda**: Subcadena del texto descifrado que se espera encontrar (opcional en algunas versiones).
4. **Número de bloques**: Solo para la versión de división en bloques, este parámetro especifica la cantidad de divisiones.

### Ejemplo:

Para ejecutar la versión `dbloques` con 4 procesos MPI y buscar una subcadena específica:

```bash
mpirun --allow-run-as-root -np 4 ./dbloques medium.txt 18014398519481984 "and typesetting industry" 1
```

## Limpieza

Para eliminar los ejecutables generados durante la compilación, puedes usar el siguiente comando:

```bash
make clean
```

## Archivos Importantes

- **naive.c**: Implementación Naive con MPI.
- **omp.c**: Implementación usando OpenMP.
- **dbloques.c**: Implementación que divide el rango de búsqueda en bloques.
- **secuencial.c**: Implementación secuencial sin paralelización.
- **medium.txt**: Archivo de texto usado para las pruebas de cifrado y descifrado.
- **Makefile**: Archivo para la compilación del proyecto.

## Notas Importantes

1. **MPI_Irecv** y **MPI_Send**: Se utilizan para la comunicación no bloqueante entre procesos MPI, permitiendo que un proceso detenga la búsqueda cuando otro ha encontrado la clave correcta.
  
2. **OpenMP**: Permite el uso de múltiples hilos para optimizar la búsqueda dentro de un proceso MPI.

## Referencias

- [OpenSSL Documentation](https://www.openssl.org/docs/)
- [MPI Tutorial](https://mpitutorial.com/)
