#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include "hash.h"

// Debug logging function
void print_debug_info(const char* filename, const unsigned char* hash, int hash_size) {
    printf("[DEBUG] Filename: %s\n", filename);
    printf("[DEBUG] Hash value: ");
    for (int i = 0; i < hash_size; i++) {
        printf("%02x", hash[i]);
    }
    printf("\n");
}

// Convert hex string to bytes
int hex_to_bytes(const char* hex, unsigned char* bytes, int size) {
    for (int i = 0; i < size; i++) {
        if (sscanf(hex + (i * 2), "%02x", &bytes[i]) != 1) {
            return -1;
        }
    }
    return 0;
}

// Hash check function
int check_hash(const char* filename) {
    FILE* hash_file;
    char line[1024];
    char stored_filename[MAX_PATH];  // MAX_PATH로 변경
    unsigned char stored_hash[HASH_SIZE];
    unsigned char current_hash[HASH_SIZE];
    char full_path[MAX_PATH];
    
    printf("[DEBUG] File to check: %s\n", filename);
    
    // Open hash.bin file
    hash_file = fopen("hash.bin", "r");
    if (hash_file == NULL) {
        printf("[DEBUG] Cannot open hash.bin file\n");
        return -1;
    }
    printf("[DEBUG] Successfully opened hash.bin file\n");

    // Read each line
    while (fgets(line, sizeof(line), hash_file)) {
        // Parse line: [filename]:hash
        char* start = strchr(line, '[');
        char* end = strchr(line, ']');
        if (!start || !end) continue;
        
        // Extract filename
        int filename_len = end - start - 1;
        if (filename_len >= MAX_PATH) filename_len = MAX_PATH - 1;  // 버퍼 오버플로우 방지
        strncpy(stored_filename, start + 1, filename_len);
        stored_filename[filename_len] = '\0';
        
        // Extract hash
        char* hash_start = strchr(line, ':') + 1;
        if (!hash_start) continue;
        
        printf("[DEBUG] Stored filename: %s\n", stored_filename);
        
        // Check if filename matches
        if (strcmp(stored_filename, filename) == 0) {
            printf("[DEBUG] Filename matches\n");
            
            // Convert hex hash to bytes
            if (hex_to_bytes(hash_start, stored_hash, HASH_SIZE) != 0) {
                printf("[DEBUG] Failed to convert hash string to bytes\n");
                fclose(hash_file);
                return -1;
            }
            
            print_debug_info(stored_filename, stored_hash, HASH_SIZE);
            
            // Calculate current file's hash
            if (GetFullPathNameA(filename, MAX_PATH, full_path, NULL) == 0) {
                printf("[DEBUG] Failed to get full path\n");
                fclose(hash_file);
                return -1;
            }
            printf("[DEBUG] Full path: %s\n", full_path);
            
            if (calculate_file_hash(full_path, current_hash) != 0) {
                printf("[DEBUG] Failed to calculate current file's hash\n");
                fclose(hash_file);
                return -1;
            }
            
            printf("[DEBUG] Successfully calculated current file's hash\n");
            print_debug_info(filename, current_hash, HASH_SIZE);

            // Compare hash values
            if (memcmp(stored_hash, current_hash, HASH_SIZE) == 0) {
                printf("[DEBUG] Hash values match\n");
                fclose(hash_file);
                return 1;
            } else {
                printf("[DEBUG] Hash values do not match\n");
                fclose(hash_file);
                return 0;
            }
        }
    }

    printf("[DEBUG] No matching file found\n");
    fclose(hash_file);
    return -1;
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        printf("Usage: %s <filename>\n", argv[0]);
        return 1;
    }

    printf("[DEBUG] Program started\n");
    printf("[DEBUG] Input filename: %s\n", argv[1]);

    int result = check_hash(argv[1]);
    
    switch (result) {
        case 1:
            printf("Hash values match.\n");
            break;
        case 0:
            printf("Hash values do not match.\n");
            break;
        case -1:
            printf("File not found or error occurred.\n");
            break;
    }

    return 0;
}
