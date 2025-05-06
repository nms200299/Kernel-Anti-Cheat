#ifndef HASH_H
#define HASH_H

#include <windows.h>
#include <wincrypt.h>

// SHA-256 Algorithm ID
#ifndef CALG_SHA_256
#define CALG_SHA_256 0x0000800c
#endif

#define HASH_SIZE 32  // SHA-256 hash size
#define BUFFER_SIZE 8192

// 파일 해시 계산 함수
int calculate_file_hash(const char* filename, unsigned char* hash);

#endif // HASH_H 