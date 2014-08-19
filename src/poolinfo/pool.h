#pragma once

//
// structure definitions
//
typedef struct _PRINT_OPTS
{
    BOOL        bRaw;
    BOOL        bHighlight;
    BOOL        bTabular;
    BOOL        bPrintIfEmpty;
    BOOL        bSummary;
    ULONG       indentLevel;
    WDBG_PTR    kPtr;
    ULONG       count;
}
PRINT_OPTS, *PPRINT_OPTS;

typedef struct _FIND_PTR
{
    WDBG_PTR    kPointer;
    WDBG_PTR    kChunk;
    // list heads stuff
    ULONG       bucket;
    ULONG       count;
    WDBG_PTR    kDesc;
}
FIND_PTR, *PFIND_PTR;

// structure to keep track of a pool descriptor
typedef struct _POOL_DESC
{
    WDBG_PTR    kPoolDesc;
    ULONG       poolType;
}
POOL_DESC, *PPOOL_DESC;

// structure to keep track of a lookaside descriptor
typedef struct _LOOKASIDE_DESC
{
    WDBG_PTR    kLookasideDesc;
    ULONG       poolType;
    ULONG       processor;
    ULONG       numBuckets;
    ULONG       structSize;
}
LOOKASIDE_DESC, *PLOOKASIDE_DESC;

//
// macros
//
#define PTR_IN_CHUNK(ctx, ptr, chunk, blocksize) \
    ( ( (ptr) >= (chunk) ) && ( (ptr) < ( (chunk) + ((blocksize) * (ctx)->poolBlockSize) ) ) )

//
// functions
//

typedef BOOL (CALLBACK* ENUMPAGEPROC)(PCMD_CTX ctx, WDBG_PTR kChunk, PVOID pHeader, LPARAM lParam);
typedef BOOL (CALLBACK* ENUMLISTHEADSPROC)(PCMD_CTX ctx, ULONG bucket, BOOL bEmpty, WDBG_PTR kChunk, ULONG count, LPARAM lParam);
typedef BOOL (CALLBACK* ENUMLOOKASIDEPROC)(PCMD_CTX ctx, WDBG_PTR kLookasideBucket, ULONG bucket, BOOL bEmpty, WDBG_PTR kChunk, ULONG count, LPARAM lParam);


ULONG GetBlockSize(PCMD_CTX ctx, PVOID pHeader);
void GetPoolTag(ULONG32 tagDword, char *tag);
const char *PoolTypeName(POOL_TYPE poolType);
const char *GetPoolType(PCMD_CTX ctx, WDBG_PTR kPoolDescriptor, ULONG *pPoolIndex);
const char *GetLookasideType(PCMD_CTX ctx, WDBG_PTR kLookaside);
const char *GetLookasideInfoType(ULONG poolType);


//
// get the paged pool info
//
POOL_DESC *GetPagedPools(PCMD_CTX ctx, PDWORD numPools);

//
// get the non-paged pools
// this is from tarjei mandt's kernel pool paper...
//
POOL_DESC *GetNonPagedPools(PCMD_CTX ctx, PDWORD numPools);

WDBG_PTR GetSessionLookaside(PCMD_CTX ctx);
WDBG_PTR GetSessionPool(PCMD_CTX ctx, PWDBG_PTR pkptrLookaside);

POOL_DESC *GetAllPoolDescriptors(PCMD_CTX ctx, ULONG *pNum);


// Get the kernel pointer to the specified field of the current KPRCB 
WDBG_PTR GetPrcbFieldOffset(PCMD_CTX ctx, WDBG_PTR kprcb, const char *fieldName);

//
// fetch the specified lookaside from the list KPRCB
//
LOOKASIDE_DESC *GetKPRCBLookasides(PCMD_CTX ctx, const char *lookasideName, ULONG lookasideType, PULONG pNum);
LOOKASIDE_DESC *GetNonPagedLookasides(PCMD_CTX ctx, PULONG pNum);
LOOKASIDE_DESC *GetNonPagedNxLookasides(PCMD_CTX ctx, PULONG pNum);
LOOKASIDE_DESC *GetPagedLookasides(PCMD_CTX ctx, PULONG pNum);
LOOKASIDE_DESC *GetSessionLookasides(PCMD_CTX ctx, PULONG pNum);
LOOKASIDE_DESC *GetAllLookasides(PCMD_CTX ctx, PULONG pNum);




void PrintTabularChunkHeader(PCMD_CTX ctx, ULONG indentLevel);

template <typename HeaderType>
HRESULT PrintPoolHeaderTempl(PCMD_CTX ctx, WDBG_PTR kPtrChunk, HeaderType *hdr, BOOL bTabular, BOOL bRaw, ULONG indentLevel, BOOL bHighlight);
HRESULT PrintPoolHeader(PCMD_CTX ctx, WDBG_PTR kPtrChunk, PVOID hdr, BOOL bTabular, BOOL bRaw, ULONG indentLevel, BOOL bHighlight);

BOOL EnumPoolPage(PCMD_CTX ctx, WDBG_PTR kPtr, LPARAM lParam, ENUMPAGEPROC enumProc);

BOOL CALLBACK EnumProcPrintHeader(PCMD_CTX ctx, WDBG_PTR kChunk, PVOID pHeader, LPARAM lParam);

BOOL EnumListHeadsBucket(PCMD_CTX ctx, WDBG_PTR kPoolDesc, ULONG bucket, LPARAM lParam, ENUMLISTHEADSPROC enumProc);
BOOL EnumListHeads(PCMD_CTX ctx, WDBG_PTR kPoolDesc, ULONG bucket, LPARAM lParam, ENUMLISTHEADSPROC enumProc);

BOOL EnumLookasideBucket(PCMD_CTX ctx, LOOKASIDE_DESC *pLookasideDesc, ULONG bucket, LPARAM lParam, ENUMLOOKASIDEPROC enumProc);
BOOL EnumLookasides(PCMD_CTX ctx, LOOKASIDE_DESC *pLookasideDesc, ULONG bucket, LPARAM lParam, ENUMLOOKASIDEPROC enumProc);

WDBG_PTR FindPoolHeader(PCMD_CTX ctx, WDBG_PTR kPtr);
BOOL FindChunkInListHeads(PCMD_CTX ctx, WDBG_PTR kPoolDesc, WDBG_PTR kPtr);

BOOL FindChunkInLookasideBucket(PCMD_CTX ctx, WDBG_PTR kLookasideBucket, WDBG_PTR kPtr, ULONG bucket, WDBG_PTR kLookaside, const char *lookasideType);
BOOL FindChunkInLookaside(PCMD_CTX ctx, WDBG_PTR kLookaside, ULONG lookasideStructSize, ULONG lookasideBuckets, WDBG_PTR kPtr, const char *lookasideType);
BOOL FindChunkInLookasides(PCMD_CTX ctx, WDBG_PTR *kLookasides, ULONG numLookasides, ULONG lookasideStructSize, ULONG lookasideBuckets, WDBG_PTR kPtr, const char *lookasideType);
BOOL FindFreeChunk(PCMD_CTX ctx, WDBG_PTR kPtr);

HRESULT PrintExtendedChunkInfo(PCMD_CTX ctx, WDBG_PTR kPtr);
