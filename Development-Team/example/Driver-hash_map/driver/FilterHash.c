#include "pch.h"
#include "FilterHash.h"

LIST_ENTRY g_HashTable[HASH_BUCKETS];

static ULONG HashUnicodeString(_In_ PUNICODE_STRING str) {
    ULONG hash = 0;
    for (USHORT i = 0; i < str->Length / 2; i++) {
        hash = 31 * hash + str->Buffer[i];
    }
    return hash % HASH_BUCKETS;
}

VOID InitFilterHash() {
    for (int i = 0; i < HASH_BUCKETS; i++) {
        InitializeListHead(&g_HashTable[i]);
    }
}

BOOLEAN AddFilterEntry(_In_ PCWSTR Path, _In_ int Action) {
    UNICODE_STRING uPath;
    RtlInitUnicodeString(&uPath, Path);

    FILTER_ENTRY* entry = ExAllocatePoolWithTag(NonPagedPool, sizeof(FILTER_ENTRY), 'htlf');
    if (!entry) return FALSE;

    entry->Path.Buffer = ExAllocatePoolWithTag(NonPagedPool, uPath.Length, 'htlf');
    if (!entry->Path.Buffer) {
        ExFreePoolWithTag(entry, 'htlf');
        return FALSE;
    }

    RtlCopyMemory(entry->Path.Buffer, uPath.Buffer, uPath.Length);
    entry->Path.Length = uPath.Length;
    entry->Path.MaximumLength = uPath.Length;
    entry->Action = Action;

    ULONG idx = HashUnicodeString(&uPath);
    InsertHeadList(&g_HashTable[idx], &entry->ListEntry);
    return TRUE;
}

FILTER_ENTRY* LookupFilterEntry(_In_ PUNICODE_STRING Path) {
    ULONG idx = HashUnicodeString(Path);
    PLIST_ENTRY head = &g_HashTable[idx];

    for (PLIST_ENTRY cur = head->Flink; cur != head; cur = cur->Flink) {
        FILTER_ENTRY* entry = CONTAINING_RECORD(cur, FILTER_ENTRY, ListEntry);
        if (RtlEqualUnicodeString(&entry->Path, Path, TRUE)) {
            return entry;
        }
    }
    return NULL;
}

VOID FreeAllFilterEntries() {
    for (int i = 0; i < HASH_BUCKETS; i++) {
        PLIST_ENTRY head = &g_HashTable[i];
        while (!IsListEmpty(head)) {
            PLIST_ENTRY entry = RemoveHeadList(head);
            FILTER_ENTRY* filter = CONTAINING_RECORD(entry, FILTER_ENTRY, ListEntry);
            ExFreePoolWithTag(filter->Path.Buffer, 'htlf');
            ExFreePoolWithTag(filter, 'htlf');
        }
    }
}
