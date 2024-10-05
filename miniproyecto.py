import concurrent.futures
import random

def suma_sublista(sublista):
    return sum(sublista)


def fork_join_suma(lista):
    num_hilos = 4
    
    tamaño_sublista = len(lista) // num_hilos
    sublistas = [lista[i * tamaño_sublista: (i + 1) * tamaño_sublista] for i in range(num_hilos)]
    
    # Procesar las sublistas en paralelo
    with concurrent.futures.ThreadPoolExecutor(max_workers=num_hilos) as executor:
        # 'Fork': Enviar cada sublista a un hilo para ser procesada
        resultados_parciales = list(executor.map(suma_sublista, sublistas))
    
    # 'Join': Combinar los resultados parciales
    suma_total = sum(resultados_parciales)
    
    return suma_total

# Simulación de una lista de 100 elementos aleatorios
if __name__ == "__main__":
    lista = [random.randint(1, 100) for _ in range(100)]
    print("Lista:", lista)
    suma = fork_join_suma(lista)
    print(f"La suma total de la lista es: {suma}")
