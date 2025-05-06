#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include <wincrypt.h>
#include "hash.h"

// File hash calculation function
int calculate_file_hash(const char* filename, unsigned char* hash) {
    HCRYPTPROV hProv = 0;
    HCRYPTHASH hHash = 0;
    FILE *file = NULL;
    BYTE buffer[BUFFER_SIZE];
    DWORD bytesRead;
    DWORD hashSize = HASH_SIZE;

    printf("[DEBUG] Starting hash calculation - File: %s\n", filename);

    // Open file
    file = fopen(filename, "rb");
    if (!file) {
        printf("[DEBUG] Cannot open file: %s\n", filename);
        return -1;
    }
    printf("[DEBUG] Successfully opened file\n");

    // Initialize crypto context
    if (!CryptAcquireContext(&hProv, NULL, NULL, PROV_RSA_AES, CRYPT_VERIFYCONTEXT)) {
        DWORD error = GetLastError();
        printf("[DEBUG] Failed to initialize crypto context. Error: %lu\n", error);
        fclose(file);
        return -1;
    }
    printf("[DEBUG] Successfully initialized crypto context\n");

    // Create hash object
    if (!CryptCreateHash(hProv, CALG_SHA_256, 0, 0, &hHash)) {
        DWORD error = GetLastError();
        printf("[DEBUG] Failed to create hash object. Error: %lu\n", error);
        CryptReleaseContext(hProv, 0);
        fclose(file);
        return -1;
    }
    printf("[DEBUG] Successfully created hash object\n");

    // Read file and calculate hash
    printf("[DEBUG] Starting file reading and hash calculation\n");
    while ((bytesRead = fread(buffer, 1, BUFFER_SIZE, file)) > 0) {
        if (!CryptHashData(hHash, buffer, bytesRead, 0)) {
            DWORD error = GetLastError();
            printf("[DEBUG] Failed to process hash data. Error: %lu\n", error);
            break;
        }
    }
    printf("[DEBUG] Completed file reading and hash calculation\n");

    // Get hash value
    if (!CryptGetHashParam(hHash, HP_HASHVAL, hash, &hashSize, 0)) {
        DWORD error = GetLastError();
        printf("[DEBUG] Failed to get hash value. Error: %lu\n", error);
        if (hHash) CryptDestroyHash(hHash);
        if (hProv) CryptReleaseContext(hProv, 0);
        fclose(file);
        return -1;
    }
    printf("[DEBUG] Successfully got hash value\n");

    // Clean up resources
    if (hHash) CryptDestroyHash(hHash);
    if (hProv) CryptReleaseContext(hProv, 0);
    fclose(file);

    printf("[DEBUG] Hash calculation completed\n");
    return 0;
} 