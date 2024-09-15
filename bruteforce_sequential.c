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
    if (argc != 5)
    {
        fprintf(stderr, "Usage: %s <input file> <output file> <encryption key> <search phrase>\n", argv[0]);
        return 1;
    }

    const char *inputFile = argv[1];
    const char *outputFile = argv[2];
    long encryptionKey = atol(argv[3]);
    const char *searchPhrase = argv[4];
    char plaintext[1024], decrypted[1024];
    unsigned char ciphertext[1024];
    int len, found = 0;

    FILE *fp = fopen(inputFile, "r");
    if (!fp)
    {
        perror("Failed to open file");
        return 1;
    }
    len = fread(plaintext, 1, sizeof(plaintext), fp);
    fclose(fp);
    encrypt(plaintext, encryptionKey, ciphertext, len);

    double startTime = clock();
    for (long key = 0; key < 72057594037927935L; key++)
    {
        if (decrypt(ciphertext, key, decrypted, len, searchPhrase))
        {
            double endTime = clock();
            found = 1;
            printf("Found key: %ld\n", key);
            printf("Decrypted text: %s\n", decrypted);
            printf("Time to find key: %f seconds\n", (endTime - startTime) / CLOCKS_PER_SEC);

            FILE *fp = fopen(outputFile, "w");
            fprintf(fp, "Key: %ld\nEncrypted text: %s\nDecrypted text: %s\nTime to decrypt: %f seconds\n",
                    key, ciphertext, decrypted, (endTime - startTime) / CLOCKS_PER_SEC);
            fclose(fp);
            break;
        }
    }
    if (!found)
    {
        printf("Key not found within the given range.\n");
    }

    return 0;
}
