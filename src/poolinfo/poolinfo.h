#pragma once

#ifdef _DEBUG
# define DEBUGPRINT dprintf
#else
# define DEBUGPRINT(...) __noop
#endif

extern BOOL g_bDebug;

// this is stupid but the api wants a sign extended 32-bit pointer
#define SIGN_EXTEND32(x)        ((ULONG64)((LONG32)(x)))


// Pool stuff
#define PAGE_SIZE               (0x1000)
#define PAGE_MASK               (~(PAGE_SIZE-1))
#define PAGE_ALIGN(p)           ( ((WDBG_PTR)(p)) & PAGE_MASK )

#define POOL_INUSE              0x2
#define POOL_INUSE_XP           0x4


#define POOL_BLOCK_SIZE32       0x8
#define POOL_BLOCK_SIZE64       0x10

// 32-bit POOL_HEADER
typedef struct _POOL_HEADER32
{
    union {
        struct {
            ULONG PreviousSize: 9;
            ULONG PoolIndex: 7;
            ULONG BlockSize: 9;
            ULONG PoolType: 7;
        };
        ULONG ULong1;
    };
    ULONG PoolTag;
} POOL_HEADER32, *PPOOL_HEADER32;

// 64-bit pool header
typedef struct _POOL_HEADER64
{
    union {
        struct {
            BYTE PreviousSize;
            BYTE PoolIndex;
            BYTE BlockSize;
            BYTE PoolType;
        };
        ULONG ULong1;
    };
    ULONG PoolTag;
} POOL_HEADER64, *PPOOL_HEADER64;

#define POOL_HEADER_SIZE        0x8

#define BUCKET_TO_SIZE(bucket, blockSize)   (((bucket) + 1) * (blockSize))

#define ROUND_POOL_SIZE(size, blockSize)    ( ((size) + ((blockSize) - 1)) & ~((blockSize) - 1) )
#define SIZE_TO_BUCKET(size, bs)            ( (ROUND_POOL_SIZE((size), (bs)) < (bs)) ? 0 : ((ROUND_POOL_SIZE((size), (bs)) / (bs)) - 1) )

typedef enum _POOL_TYPE { 
  NonPagedPool,
  NonPagedPoolExecute                   = NonPagedPool,
  PagedPool,
  NonPagedPoolMustSucceed               = NonPagedPool + 2,
  DontUseThisType,
  NonPagedPoolCacheAligned              = NonPagedPool + 4,
  PagedPoolCacheAligned,
  NonPagedPoolCacheAlignedMustS         = NonPagedPool + 6,
  MaxPoolType,
  NonPagedPoolBase                      = 0,
  NonPagedPoolBaseMustSucceed           = NonPagedPoolBase + 2,
  NonPagedPoolBaseCacheAligned          = NonPagedPoolBase + 4,
  NonPagedPoolBaseCacheAlignedMustS     = NonPagedPoolBase + 6,
  NonPagedPoolSession                   = 32,
  PagedPoolSession                      = NonPagedPoolSession + 1,
  NonPagedPoolMustSucceedSession        = PagedPoolSession + 1,
  DontUseThisTypeSession                = NonPagedPoolMustSucceedSession + 1,
  NonPagedPoolCacheAlignedSession       = DontUseThisTypeSession + 1,
  PagedPoolCacheAlignedSession          = NonPagedPoolCacheAlignedSession + 1,
  NonPagedPoolCacheAlignedMustSSession  = PagedPoolCacheAlignedSession + 1,
  NonPagedPoolNx                        = 512,
  NonPagedPoolNxCacheAligned            = NonPagedPoolNx + 4,
  NonPagedPoolSessionNx                 = NonPagedPoolNx + 32,
} POOL_TYPE;


// size of the lookaside struct for session
// i hope this is pretty portable...
#define SESSION_LOOKASIDE_SIZE          0x80

#define POOL_LOOKASIDE_BUCKETS          0x20


//
// options stuff
//

#define POOL_TYPE_NONPAGED  0x1
#define POOL_TYPE_PAGED     0x2
#define POOL_TYPE_SESSION   0x4

#define BUCKET_ALL          ((ULONG)-1)

// command-line option ids
enum {
    OPT_HELP,
    OPT_PP_RAWHDR,
    OPT_PL_ALL,
    OPT_PL_NONPAGED,
    OPT_PL_PAGED,
    OPT_PL_SESSION,
    OPT_PL_LOOKASIDE,
    OPT_PI_SUMMARY,
    OPT_PI_VERBOSE,
    OPT_PI_FREELIST,
    OPT_PI_BUCKET,
    OPT_PI_LOOKASIDE,
    OPT_PI_LOOKTYPE,
    OPT_PI_NUMBUCKETS,
    OPT_PI_LOOKSIZE
};

#define HELP_OPTS \
    { OPT_HELP,         "-h",       SO_NONE }, \
    { OPT_HELP,         "--help",   SO_NONE }, \
    { OPT_HELP,         "-?",       SO_NONE }



