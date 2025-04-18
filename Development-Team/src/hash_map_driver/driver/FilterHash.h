#pragma once
#include <fltKernel.h>

#define HASH_BUCKETS 37
#define FILTER_ACTION_BLOCK 0
#define FILTER_ACTION_LOG   1
#define FILTER_ACTION_ALLOW 2

typedef struct _FILTER_ENTRY {
    LIST_ENTRY ListEntry;
    UNICODE_STRING Path;
    int Action;
} FILTER_ENTRY;

extern LIST_ENTRY g_HashTable[HASH_BUCKETS];

VOID InitFilterHash();
BOOLEAN AddFilterEntry(_In_ PCWSTR Path, _In_ int Action);
FILTER_ENTRY* LookupFilterEntry(_In_ PUNICODE_STRING Path);
VOID FreeAllFilterEntries();
