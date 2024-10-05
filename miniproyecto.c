#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#define NUM_HILOS 4 // Número de hilos
#define N 100       // Tamaño de la lista

// Estructura para pasar datos a cada hilo
typedef struct
{
  int *sublista;
  int tamaño;
  long long suma_parcial;
  int id_hilo; // Identificador del hilo
} DatosHilo;

// Función que será ejecutada por cada hilo para sumar los elementos de una sublista
void *suma_sublista(void *args)
{
  DatosHilo *datos = (DatosHilo *)args;
  datos->suma_parcial = 0;

  // Mostrar qué lista tiene este hilo
  printf("Hilo %d procesando los elementos: ", datos->id_hilo);
  for (int i = 0; i < datos->tamaño; i++)
  {
    printf("%d ", datos->sublista[i]);
    datos->suma_parcial += datos->sublista[i];
  }
  printf("\n");

  printf("Suma parcial del hilo %d: %lld\n", datos->id_hilo, datos->suma_parcial);

  pthread_exit(NULL);
}

int main()
{
  int lista[N];
  pthread_t hilos[NUM_HILOS];
  DatosHilo datos[NUM_HILOS];
  long long suma_total = 0;
  int tamaño_sublista = N / NUM_HILOS; // Dividir la lista en partes iguales

  // Inicializar la lista con valores aleatorios
  for (int i = 0; i < N; i++)
  {
    lista[i] = rand() % 100;
  }

  // Mostrar la lista completa
  printf("Lista completa:\n");
  for (int i = 0; i < N; i++)
  {
    printf("%d ", lista[i]);
  }
  printf("\n");

  // Crear hilos para procesar cada sublista
  for (int i = 0; i < NUM_HILOS; i++)
  {
    datos[i].sublista = &lista[i * tamaño_sublista];
    datos[i].tamaño = (i == NUM_HILOS - 1) ? (N - i * tamaño_sublista) : tamaño_sublista; // Ajuste para el último hilo
    datos[i].id_hilo = i + 1;                                                             // Asignar identificador de hilo
    pthread_create(&hilos[i], NULL, suma_sublista, (void *)&datos[i]);
  }

  // Esperar a que todos los hilos terminen
  for (int i = 0; i < NUM_HILOS; i++)
  {
    pthread_join(hilos[i], NULL);
    suma_total += datos[i].suma_parcial; // Combinar los resultados parciales
  }

  // Imprimir el resultado final
  printf("La suma total de la lista es: %lld\n", suma_total);

  return 0;
}
