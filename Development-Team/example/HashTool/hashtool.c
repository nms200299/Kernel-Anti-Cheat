#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include <wincrypt.h>
#include <direct.h>
#include <io.h>

#define BUFFER_SIZE 8192
#define MAX_PATH_LENGTH MAX_PATH

// Check admin privileges
BOOL IsElevated() {
    BOOL fRet = FALSE;
    HANDLE hToken = NULL;
    if (OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken)) {
        TOKEN_ELEVATION Elevation;
        DWORD cbSize = sizeof(TOKEN_ELEVATION);
        if (GetTokenInformation(hToken, TokenElevation, &Elevation, sizeof(Elevation), &cbSize)) {
            fRet = Elevation.TokenIsElevated;
        }
    }
    if (hToken) {
        CloseHandle(hToken);
    }
    return fRet;
}

// Calculate file hash
BOOL CalculateFileHash(LPCSTR filename, BYTE *hash, DWORD *hashSize) {
    HCRYPTPROV hProv = 0;
    HCRYPTHASH hHash = 0;
    BOOL result = FALSE;
    FILE *file = NULL;
    BYTE buffer[BUFFER_SIZE];
    DWORD bytesRead;

    // Open file
    file = fopen(filename, "rb");
    if (!file) {
        printf("[ERROR] Cannot open file: %s\n", filename);
        return FALSE;
    }

    // Initialize crypto context
    if (!CryptAcquireContext(&hProv, NULL, NULL, PROV_RSA_AES, CRYPT_VERIFYCONTEXT)) {
        DWORD error = GetLastError();
        printf("[ERROR] Failed to initialize crypto context. Error: %lu\n", error);
        fclose(file);
        return FALSE;
    }

    // Create hash object
    if (!CryptCreateHash(hProv, CALG_SHA_256, 0, 0, &hHash)) {
        DWORD error = GetLastError();
        printf("[ERROR] Failed to create hash object. Error: %lu\n", error);
        CryptReleaseContext(hProv, 0);
        fclose(file);
        return FALSE;
    }

    // Read file and calculate hash
    while ((bytesRead = fread(buffer, 1, BUFFER_SIZE, file)) > 0) {
        if (!CryptHashData(hHash, buffer, bytesRead, 0)) {
            DWORD error = GetLastError();
            printf("[ERROR] Failed to process hash data. Error: %lu\n", error);
            break;
        }
    }

    // Get hash value
    *hashSize = 32; // SHA-256 hash size
    if (CryptGetHashParam(hHash, HP_HASHVAL, hash, hashSize, 0)) {
        result = TRUE;
    } else {
        DWORD error = GetLastError();
        printf("[ERROR] Failed to get hash value. Error: %lu\n", error);
    }

    // Clean up resources
    if (hHash) CryptDestroyHash(hHash);
    if (hProv) CryptReleaseContext(hProv, 0);
    fclose(file);

    return result;
}

int main() {
    // Check admin privileges and auto-run
    if (!IsElevated()) {
        char path[MAX_PATH];
        if (GetModuleFileName(NULL, path, MAX_PATH) == 0) {
            printf("[ERROR] Failed to get module path\n");
            return 1;
        }
        ShellExecute(NULL, "runas", path, NULL, NULL, SW_HIDE);
        return 0;
    }

    // Search all files in current directory
    struct _finddata_t fileinfo;
    long handle;
    char path[MAX_PATH_LENGTH];
    BYTE hash[32];
    DWORD hashSize;
    FILE *outputFile;

    // Open output file
    outputFile = fopen("hash.bin", "w");
    if (!outputFile) {
        printf("[ERROR] Cannot create output file\n");
        return 1;
    }

    // Search all files in current directory
    strcpy(path, ".\\*");
    handle = _findfirst(path, &fileinfo);
    
    if (handle != -1) {
        do {
            if (!(fileinfo.attrib & _A_SUBDIR)) { // Process only files, not directories
                // Exclude hash.bin and hashtool.exe
                if (strcmp(fileinfo.name, "hash.bin") == 0 || strcmp(fileinfo.name, "hashtool.exe") == 0) {
                    continue;
                }

                char fullPath[MAX_PATH_LENGTH];
                if (snprintf(fullPath, MAX_PATH_LENGTH, ".\\%s", fileinfo.name) >= MAX_PATH_LENGTH) {
                    printf("[WARNING] Path too long, skipping: %s\n", fileinfo.name);
                    continue;
                }
                
                printf("[INFO] Processing file: %s\n", fileinfo.name);
                
                if (CalculateFileHash(fullPath, hash, &hashSize)) {
                    // Write filename and hash value to output file
                    fprintf(outputFile, "[%s]:", fileinfo.name);
                    for (DWORD i = 0; i < hashSize; i++) {
                        fprintf(outputFile, "%02x", hash[i]);
                    }
                    fprintf(outputFile, "\n");
                    printf("[INFO] Successfully processed: %s\n", fileinfo.name);
                } else {
                    printf("[ERROR] Failed to process: %s\n", fileinfo.name);
                }
            }
        } while (_findnext(handle, &fileinfo) == 0);
        _findclose(handle);
    }

    fclose(outputFile);
    printf("[INFO] Hash generation completed\n");
    return 0;
}
