#include <omp.h>
#include <stdio.h>

#define N 100000000 // 100 millones de registros

// Simulación del procesamiento de un registro
int process_record(int i)
{
  // En este ejemplo, simplemente retornamos un valor simulado
  return i % 2; // Por ejemplo, devuelve 1 si i es impar y 0 si es par
}

int main()
{
  int i;
  long long total_sum = 0; // Variable para almacenar el resultado final

// Paralelización utilizando OpenMP y reducción para sumar los resultados parciales
#pragma omp parallel for reduction(+ : total_sum)
  for (i = 0; i < N; i++)
  {
    total_sum += process_record(i); // Suma el resultado de procesar cada registro
  }

  // Imprimir el resultado final
  printf("Resultado final: %lld\n", total_sum);
  return 0;
}
