#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <openssl/des.h>
#include <time.h>

void encrypt(const char *plaintext, long key, unsigned char *ciphertext, int len)
{
    DES_cblock keyBlock;
    DES_key_schedule schedule;
    memcpy(&keyBlock, &key, sizeof(key));
    DES_set_key_unchecked(&keyBlock, &schedule);
    for (int i = 0; i < len; i += 8)
    {
        DES_ecb_encrypt((DES_cblock *)(plaintext + i), (DES_cblock *)(ciphertext + i), &schedule, DES_ENCRYPT);
    }
}

int decrypt(const unsigned char *ciphertext, long key, char *decrypted, int len, const char *search)
{
    DES_cblock keyBlock;
    DES_key_schedule schedule;
    memcpy(&keyBlock, &key, sizeof(key));
    DES_set_key_unchecked(&keyBlock, &schedule);
    for (int i = 0; i < len; i += 8)
    {
        DES_ecb_encrypt((DES_cblock *)(ciphertext + i), (DES_cblock *)(decrypted + i), &schedule, DES_DECRYPT);
    }
    decrypted[len] = '\0';
    return strstr(decrypted, search) != NULL;
}

int main(int argc, char *argv[])
{
    int my_rank, num_procs;
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
    MPI_Comm_size(MPI_COMM_WORLD, &num_procs);

    if (argc != 5)
    {
        if (my_rank == 0)
        {
            fprintf(stderr, "Usage: %s <input file> <output file> <encryption key> <search phrase>\n", argv[0]);
        }
        MPI_Finalize();
        return 1;
    }

    const char *inputFile = argv[1];
    const char *outputFile = argv[2];
    long encryptionKey = atol(argv[3]);
    const char *searchPhrase = argv[4];
    char plaintext[1024], decrypted[1024];
    unsigned char ciphertext[1024];
    int len, global_found = 0;

    double startTime = MPI_Wtime();

    if (my_rank == 0)
    {
        FILE *fp = fopen(inputFile, "r");
        if (!fp)
        {
            perror("Failed to open file");
            MPI_Abort(MPI_COMM_WORLD, 1);
        }
        len = fread(plaintext, 1, sizeof(plaintext), fp);
        fclose(fp);
        encrypt(plaintext, encryptionKey, ciphertext, len);
    }

    MPI_Bcast(&len, 1, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(ciphertext, len, MPI_UNSIGNED_CHAR, 0, MPI_COMM_WORLD);

    long keyStart = my_rank * (72057594037927935L / num_procs);
    long keyEnd = (my_rank + 1) * (72057594037927935L / num_procs);
    if (my_rank == num_procs - 1)
        keyEnd = 72057594037927935L; // Last process takes any remainder

    for (long key = keyStart; !global_found && key < keyEnd; key++)
    {
        if (decrypt(ciphertext, key, decrypted, len, searchPhrase))
        {
            double endTime = MPI_Wtime();
            global_found = 1;
            MPI_Bcast(&global_found, 1, MPI_INT, my_rank, MPI_COMM_WORLD); // Inform other processes
            if (my_rank == 0)
            {
                FILE *fp = fopen(outputFile, "w");
                fprintf(fp, "Key: %ld\nEncrypted text: %s\nDecrypted text: %s\nTime to decrypt: %f seconds\n",
                        key, ciphertext, decrypted, endTime - startTime);
                fclose(fp);
            }
            printf("Process %d found key: %ld\n", my_rank, key);
            printf("Decrypted text: %s\n", decrypted);
            printf("Time to find key: %f seconds\n", endTime - startTime);
        }
    }

    MPI_Finalize();
    return 0;
}
