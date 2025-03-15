#pragma once
#include "miniflt.h"


//
//  Memory Pool Tags
//

#define CTX_STRING_TAG                        'tSxC'
#define CTX_RESOURCE_TAG                      'cRxC'
//#define CTX_INSTANCE_CONTEXT_TAG              'cIxC'
//#define CTX_FILE_CONTEXT_TAG                  'cFxC'
//#define CTX_STREAM_CONTEXT_TAG                'cSxC'
#define CTX_STREAMHANDLE_CONTEXT_TAG          'cHxC'


//
//  Context sample filter global data
//

typedef struct _CTX_GLOBAL_DATA {

    //
    // Handle to minifilter returned from FltRegisterFilter()
    //

    PFLT_FILTER Filter;

} CTX_GLOBAL_DATA, * PCTX_GLOBAL_DATA;

extern CTX_GLOBAL_DATA Globals;





//
//  Stream handle context data structure
//

typedef struct _CTX_STREAMHANDLE_CONTEXT {

    //
    //  Name of the file associated with this context.
    //

    UNICODE_STRING FileName;

    //
    //  Lock used to protect this context.
    //

    PERESOURCE Resource;

} CTX_STREAMHANDLE_CONTEXT, * PCTX_STREAMHANDLE_CONTEXT;

#define CTX_STREAMHANDLE_CONTEXT_SIZE         sizeof( CTX_STREAMHANDLE_CONTEXT )

///////

//
//  Functions implemented in operations.c
//

FLT_PREOP_CALLBACK_STATUS
CtxPreCreate(
    _Inout_ PFLT_CALLBACK_DATA Cbd,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _Flt_CompletionContext_Outptr_ PVOID* CompletionContext
);

FLT_POSTOP_CALLBACK_STATUS
CtxPostCreate(
    _Inout_ PFLT_CALLBACK_DATA Cbd,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _Inout_opt_ PVOID CbdContext,
    _In_ FLT_POST_OPERATION_FLAGS Flags
);

FLT_PREOP_CALLBACK_STATUS
CtxPreCleanup(
    _Inout_ PFLT_CALLBACK_DATA Cbd,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _Flt_CompletionContext_Outptr_ PVOID* CompletionContext
);

FLT_PREOP_CALLBACK_STATUS
CtxPreClose(
    _Inout_ PFLT_CALLBACK_DATA Cbd,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _Flt_CompletionContext_Outptr_ PVOID* CompletionContext
);


//
//  Functions implemented in context.c
//
NTSTATUS
CtxCreateOrReplaceStreamHandleContext(
    _In_ PFLT_CALLBACK_DATA Cbd,
    _In_ BOOLEAN ReplaceIfExists,
    _Outptr_ PCTX_STREAMHANDLE_CONTEXT* StreamHandleContext,
    _Out_opt_ PBOOLEAN ContextReplaced
);

NTSTATUS
CtxCreateStreamHandleContext(
    _Outptr_ PCTX_STREAMHANDLE_CONTEXT* StreamHandleContext
);

NTSTATUS
CtxUpdateNameInStreamHandleContext(
    _In_ PUNICODE_STRING DirectoryName,
    _Inout_ PCTX_STREAMHANDLE_CONTEXT StreamHandleContext
);


//
//  Functions implemented in support.c
//

_At_(String->Length, _Out_range_(== , 0))
_At_(String->MaximumLength, _In_)
_At_(String->Buffer, _Pre_maybenull_ _Post_notnull_ _Post_writable_byte_size_(String->MaximumLength))
NTSTATUS
CtxAllocateUnicodeString(
    _Out_ PUNICODE_STRING String
);

_At_(String->Length, _Out_range_(== , 0))
_At_(String->MaximumLength, _Out_range_(== , 0))
_At_(String->Buffer, _Pre_notnull_ _Post_null_)
VOID
CtxFreeUnicodeString(
    _Pre_notnull_ PUNICODE_STRING String
);


//
//  Resource support
//

FORCEINLINE
PERESOURCE
CtxAllocateResource(
    VOID
)
{

    return (PERESOURCE)ExAllocatePoolZero(NonPagedPool,
        sizeof(ERESOURCE),
        CTX_RESOURCE_TAG);
}

FORCEINLINE
VOID
CtxFreeResource(
    _In_ PERESOURCE Resource
)
{

    ExFreePoolWithTag(Resource,
        CTX_RESOURCE_TAG);
}

FORCEINLINE
VOID
_Acquires_lock_(_Global_critical_region_)
_IRQL_requires_max_(APC_LEVEL)
CtxAcquireResourceExclusive(
    _Inout_ _Requires_lock_not_held_(*_Curr_) _Acquires_exclusive_lock_(*_Curr_)
    PERESOURCE Resource
)
{
    FLT_ASSERT(KeGetCurrentIrql() <= APC_LEVEL);
    FLT_ASSERT(ExIsResourceAcquiredExclusiveLite(Resource) ||
        !ExIsResourceAcquiredSharedLite(Resource));

    KeEnterCriticalRegion();
    (VOID)ExAcquireResourceExclusiveLite(Resource, TRUE);
}

FORCEINLINE
VOID
_Acquires_lock_(_Global_critical_region_)
_IRQL_requires_max_(APC_LEVEL)
CtxAcquireResourceShared(
    _Inout_ _Requires_lock_not_held_(*_Curr_) _Acquires_shared_lock_(*_Curr_)
    PERESOURCE Resource
)
{
    FLT_ASSERT(KeGetCurrentIrql() <= APC_LEVEL);

    KeEnterCriticalRegion();
    (VOID)ExAcquireResourceSharedLite(Resource, TRUE);
}

FORCEINLINE
VOID
_Releases_lock_(_Global_critical_region_)
_Requires_lock_held_(_Global_critical_region_)
_IRQL_requires_max_(APC_LEVEL)
CtxReleaseResource(
    _Inout_ _Requires_lock_held_(*_Curr_) _Releases_lock_(*_Curr_)
    PERESOURCE Resource
)
{
    FLT_ASSERT(KeGetCurrentIrql() <= APC_LEVEL);
    FLT_ASSERT(ExIsResourceAcquiredExclusiveLite(Resource) ||
        ExIsResourceAcquiredSharedLite(Resource));

    ExReleaseResourceLite(Resource);
    KeLeaveCriticalRegion();
}




#ifndef PROCESS_TERMINATE
#define PROCESS_TERMINATE (0x0001) // 수동으로 정의
#endif



// 레지스트리 관련
