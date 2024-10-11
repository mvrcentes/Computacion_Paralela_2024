#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mpi.h>
#include <openssl/des.h>
#include <time.h>

void add_padding(unsigned char *input, int *len) {
    int padding = 8 - (*len % 8);
    for (int i = 0; i < padding; ++i) {
        input[*len + i] = padding; // Rellenar con la cantidad de bytes de padding
    }
    *len += padding;
}

void remove_padding(unsigned char *input, int *len) {
    int padding = input[*len - 1];
    *len -= padding;
}

void decrypt_message(long key, unsigned char *ciph, int *len) {
    DES_cblock keyBlock;
    DES_key_schedule schedule;

    for (int i = 0; i < 8; ++i) {
        keyBlock[i] = (key >> (i * 8)) & 0xFF;
    }

    DES_set_odd_parity(&keyBlock);
    DES_set_key_checked(&keyBlock, &schedule);

    for (int i = 0; i < *len; i += 8) {
        DES_ecb_encrypt((DES_cblock *)(ciph + i), (DES_cblock *)(ciph + i), &schedule, DES_DECRYPT);
    }
    remove_padding(ciph, len);
}

void encrypt_message(long key, unsigned char *ciph, int *len) {
    DES_cblock keyBlock;
    DES_key_schedule schedule;

    for (int i = 0; i < 8; ++i) {
        keyBlock[i] = (key >> (i * 8)) & 0xFF;
    }

    DES_set_odd_parity(&keyBlock);
    DES_set_key_checked(&keyBlock, &schedule);

    add_padding(ciph, len);

    for (int i = 0; i < *len; i += 8) {
        DES_ecb_encrypt((DES_cblock *)(ciph + i), (DES_cblock *)(ciph + i), &schedule, DES_ENCRYPT);
    }
}

int tryKey(long key, unsigned char *ciph, int len, const char *search) {
    unsigned char temp[len + 1];
    memcpy(temp, ciph, len);
    temp[len] = 0; // Asegurar que la cadena esté terminada en nulo

    decrypt_message(key, temp, &len);
    temp[len] = '\0'; // Terminar la cadena descifrada explícitamente

    if (strstr((char *)temp, search) != NULL) {
        return 1; // Llave encontrada
    }
    return 0; // Llave no encontrada
}

int main(int argc, char *argv[]) {
    if (argc != 4) {
        fprintf(stderr, "Uso: %s <archivo> <llave> <cadena_a_buscar>\n", argv[0]);
        return EXIT_FAILURE;
    }

    char *filename = argv[1];
    long key = atol(argv[2]);
    char *search = argv[3];

    int N, id, flag;
    MPI_Comm comm = MPI_COMM_WORLD;
    MPI_Init(&argc, &argv);
    MPI_Comm_size(comm, &N);
    MPI_Comm_rank(comm, &id);

    // Leer el contenido del archivo
    FILE *file = fopen(filename, "rb");
    if (!file) {
        perror("No se pudo abrir el archivo");
        MPI_Finalize();
        return EXIT_FAILURE;
    }

    fseek(file, 0, SEEK_END);
    long filesize = ftell(file);
    fseek(file, 0, SEEK_SET);
    unsigned char *plaintext = malloc(filesize);
    fread(plaintext, 1, filesize, file);
    fclose(file);

    unsigned char *cipher = malloc(filesize + 8); // +8 para el padding
    int ciphlen = filesize;
    memcpy(cipher, plaintext, ciphlen);

    // Cifrar el mensaje con la llave proporcionada
    encrypt_message(key, cipher, &ciphlen);

    long upper = (1L << 56);
    long mylower = (upper / N) * id;
    long myupper = (upper / N) * (id + 1) - 1;
    if (id == N - 1) {
        myupper = upper;
    }

    printf("Proceso %d está buscando en el rango de llaves: [%li - %li]\n", id, mylower, myupper);

    long found = -1; // Inicializar como -1 para indicar que no se ha encontrado la llave
    int key_found = 0; // Bandera para indicar si se ha encontrado la llave
    double start_time = MPI_Wtime(); // Iniciar cronómetro

    // Bucle para buscar la llave
    for (long i = mylower; i <= myupper; ++i) {
        MPI_Iprobe(MPI_ANY_SOURCE, MPI_ANY_TAG, comm, &flag, MPI_STATUS_IGNORE);
        if (flag) { // Revisar si alguna llave ha sido encontrada
            MPI_Recv(&key_found, 1, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, comm, MPI_STATUS_IGNORE);
            if (key_found) {
                break; // Salir si una llave ya ha sido encontrada
            }
        }

        if (tryKey(i, cipher, ciphlen, search)) {
            found = i; // Llave encontrada
            key_found = 1; // Cambiar la bandera a que la llave ha sido encontrada
            for (int j = 0; j < N; ++j) {
                MPI_Send(&key_found, 1, MPI_INT, j, 0, comm); // Informar a todos los procesos
            }
            break;
        }
    }

    if (found != -1) {
        double end_time = MPI_Wtime(); // Terminar cronómetro
        printf("\n====================================================\n");
        printf("¡Llave encontrada!\n");
        printf("Llave: %li, encontrada por el proceso %d\n", found, id);
        decrypt_message(found, cipher, &ciphlen);
        cipher[ciphlen] = '\0'; // Asegurar que la cadena descifrada esté terminada en nulo
        printf("Texto descifrado: %s\n", cipher);
        printf("Tiempo total de descifrado: %.4f segundos\n", end_time - start_time);
        printf("====================================================\n");
    }

    free(plaintext);
    free(cipher);
    MPI_Finalize();
    return EXIT_SUCCESS;
}
