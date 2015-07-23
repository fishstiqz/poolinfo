#include "stdafx.h"


ULONG GetBlockSize(PCMD_CTX ctx, PVOID pHeader)
{
    ULONG BlockSize;

    if (ctx->b64BitTarget)
        BlockSize = ((PPOOL_HEADER64)pHeader)->BlockSize;
    else
        BlockSize = ((PPOOL_HEADER32)pHeader)->BlockSize;

    return BlockSize;
}

void GetPoolTag(ULONG32 tagDword, char *tag)
{
    int i;
    unsigned char *p = (unsigned char *)(&tagDword);

    for (i = 0; i < 4; i++)
        tag[i] = ( isprint((int)p[i]) ? p[i] : '.' );
    
    tag[4] = '\0';
}


const char *PoolTypeName(POOL_TYPE poolType)
{
#define POOL_TYPE_CASE(t) \
    case t: return #t

    switch (poolType)
    {
    POOL_TYPE_CASE(NonPagedPool);
    POOL_TYPE_CASE(PagedPool);
    POOL_TYPE_CASE(NonPagedPoolSession);
    POOL_TYPE_CASE(PagedPoolSession);
    POOL_TYPE_CASE(NonPagedPoolNx);
    POOL_TYPE_CASE(NonPagedPoolSessionNx);
    default:
        return "Unknown Pool";
    }
}

const char *GetPoolType(PCMD_CTX ctx, WDBG_PTR kPoolDescriptor, ULONG *pPoolIndex)
{
    const char *name;
    PSYM_FIELD pStructDesc;
    PVOID pStructData;

    pStructDesc = CloneStruct(ctx->pStructPoolDescriptor);
    if (pStructDesc == NULL)
        return NULL;

    if (FAILED(ReadVirtualStruct(ctx, kPoolDescriptor, pStructDesc, &pStructData)))
    {
        free(pStructDesc);
        return NULL;
    }

    name = PoolTypeName((POOL_TYPE) GS_ULONG(pStructDesc, "PoolType"));
    if (pPoolIndex != NULL)
        *pPoolIndex = GS_ULONG(pStructDesc, "PoolIndex");

    free(pStructDesc);
    free(pStructData);

    return name;
}

// return the typename of the lookaside
const char *GetLookasideType(PCMD_CTX ctx, WDBG_PTR kLookaside)
{
    const char *name;
    PSYM_FIELD pStructDesc;
    PVOID pStructData;

    pStructDesc = CloneStruct(ctx->pStructGeneralLookasidePool);
    if (pStructDesc == NULL)
        return NULL;

    if (FAILED(ReadVirtualStruct(ctx, kLookaside, pStructDesc, &pStructData)))
    {
        free(pStructDesc);
        return NULL;
    }

    name = PoolTypeName((POOL_TYPE) GS_ULONG(pStructDesc, "Type"));

    free(pStructDesc);
    free(pStructData);

    return name;
}

const char *GetLookasideInfoType(ULONG poolType)
{
    const char *infotype = "Unknown";
    
    if (poolType == POOL_TYPE_NONPAGED)
        infotype = "NonPaged";
    else if (poolType == POOL_TYPE_PAGED)
        infotype = "Paged";
    else if (poolType == POOL_TYPE_SESSION)
        infotype = "Session";

    return infotype;
}



//
// get the paged pool info
//
POOL_DESC *GetPagedPools(PCMD_CTX ctx, PDWORD numPools)
{
    DWORD num, i;
    POOL_DESC *descs;
    
    WDBG_PTR kNum = GetExpression("nt!ExpNumberOfPagedPools");
    WDBG_PTR kDescList = GetExpression("nt!ExpPagedPoolDescriptor");

    // read the number of paged pools
    if (FAILED(ReadVirtual(ctx, kNum, &num, sizeof(num))))
        return NULL;

    // add 1 to the number of paged pools
    // this accounts for the extra descriptor at ExpPagedPoolDescriptor[0]
    num++;

    descs = (POOL_DESC *) calloc(num, sizeof(POOL_DESC));
    if (descs == NULL)
        return NULL;

    // read the pool descriptor pointers
    for (i = 0; i < num; i++)
    {
        if (FAILED(ReadTargetPointer(ctx, kDescList + (i * ctx->pointerLen), &descs[i].kPoolDesc)))
        {
            free(descs);
            return NULL;
        }
        descs[i].poolType = POOL_TYPE_PAGED;
    }

    *numPools = num;
    return descs;
}


//
// get the non-paged pools
// this is from tarjei mandt's kernel pool paper...
//
POOL_DESC *GetNonPagedPools(PCMD_CTX ctx, PDWORD numPools)
{
    DWORD numProcs = 0;
    WDBG_PTR kDescList;
    WDBG_PTR kVectorBase;
    DWORD numDescs = 0;
    POOL_DESC *descs;
    DWORD i;

    // how many processors?
    // should we instead be looking at nt!ExpNumberOfNonPagedPools ?
    WDBG_PTR kNumProc = GetExpression("nt!KeNumberProcessors");
    if (FAILED(ReadVirtual(ctx, kNumProc, &numProcs, sizeof(numProcs))))
        return NULL;

    if (numProcs > 1)
    {
        // multi-processor: read from nt!ExpNonPagedPoolDescriptor
        kDescList = GetExpression("nt!ExpNonPagedPoolDescriptor");
        numDescs = numProcs;

        descs = (POOL_DESC *) calloc(numDescs, sizeof(POOL_DESC));
        if (descs == NULL)
            return NULL;

        // read the pool descriptor pointers
        for (i = 0; i < numDescs; i++)
        {
            if (FAILED(ReadTargetPointer(ctx, kDescList + (i * ctx->pointerLen), &descs[i].kPoolDesc)))
            {
                free(descs);
                return NULL;
            }
            descs[i].poolType = POOL_TYPE_NONPAGED;
        }
    }
    else
    {
        // uni-processor: read from nt!PoolVector[0]
        kVectorBase = GetExpression("nt!PoolVector");
        numDescs = 1;

        // we cannot trust osMinor on Windows 8.1 and above thanks to 
        // the deprecation of GetVersionEx.. which I assume dbgeng must be using
        // given that it returns the wrong info for 8.1...
        // However, the build number is returned correctly.
        // Any build number >= 9200 is windows 8+
        if (ctx->osBuild >= 9200)
            numDescs++;
        
        // read the descriptors pointer 
        if (FAILED(ReadTargetPointer(ctx, kVectorBase, &kDescList)))
            return NULL;

        descs = (POOL_DESC *) calloc(numDescs, sizeof(POOL_DESC));
        if (descs == NULL)
            return NULL;

        for (i = 0; i < numDescs; i++)
        {
            descs[i].kPoolDesc = kDescList;
            descs[i].poolType = POOL_TYPE_NONPAGED;

            kDescList += 0x140 + 0x1000;
        }
    }

    *numPools = numDescs;
    return descs;
}

WDBG_PTR GetSessionLookaside(PCMD_CTX ctx)
{
    WDBG_PTR kptrLookaside = 0;
    WDBG_PTR p = GetExpression("nt!ExpSessionPoolLookaside");

    if (FAILED(ReadTargetPointer(ctx, p, &kptrLookaside)))
        return NULL;

    return kptrLookaside;
}

WDBG_PTR GetSessionPool(PCMD_CTX ctx, PWDBG_PTR pkptrLookaside)
{
    WDBG_PTR pSessionPool = 0;
    ULONG64 eprocOffset;
    PSYM_FIELD pStructEproc = NULL;
    PSYM_FIELD pStructSession = NULL;
    PVOID pEprocData = NULL;

    if (pkptrLookaside != NULL)
        *pkptrLookaside = 0;

    // get the current EPROCESS
    if (FAILED(ctx->pDebugSystemObjects->GetImplicitProcessDataOffset(&eprocOffset)))
        return NULL;

    pStructEproc = CloneStruct(ctx->pStructEprocess);
    if (pStructEproc == NULL)
        return NULL;

    pStructSession = CloneStruct(ctx->pStructMmSessionSpace);
    if (pStructSession == NULL)
    {
        free(pStructEproc);
        return NULL;
    }

    // read the EPROCESS
    if (SUCCEEDED(ReadVirtualStruct(ctx, (WDBG_PTR)eprocOffset, pStructEproc, &pEprocData)))
    {
        WDBG_PTR addrSession = GetStructPointer(ctx, pStructEproc, "Session");
        
        // not every process will have a session, such as System
        if (addrSession != 0)
        {
            // get address of MM_SESSION_SPACE.PagedPool
            pSessionPool = addrSession + GetStructField(pStructSession, "PagedPool")->fieldOffset;
        
            // lookaside @ &MM_SESSION_SPACE.Lookaside
            if (pkptrLookaside != NULL)
                *pkptrLookaside = addrSession + GetStructField(pStructSession, "Lookaside")->fieldOffset;

            //DEBUGPRINT("kptrLookaside: %p\n", *pkptrLookaside);
        }

        free(pEprocData);
    }
    
    free(pStructSession);
    free(pStructEproc);

    return pSessionPool;
}

POOL_DESC *GetAllPoolDescriptors(PCMD_CTX ctx, ULONG *pNum)
{
    POOL_DESC *pools = NULL;
    ULONG count = 0;
    ULONG total = 0, i = 0;
    POOL_DESC *NPDescs, *PDescs;
    WDBG_PTR SDesc;
    DWORD numNP, numP;
    PBYTE p;

    *pNum = 0;

    NPDescs = GetNonPagedPools(ctx, &numNP);
    PDescs = GetPagedPools(ctx, &numP);
    SDesc = GetSessionPool(ctx, NULL);

    if (NPDescs != NULL) count += numNP;
    if (PDescs != NULL) count += numP;
    if (SDesc != NULL) count += 1;

    if (count == 0)
        return NULL;

    pools = (POOL_DESC *) calloc(count, sizeof(POOL_DESC));
    if (pools != NULL)
    {
        p = (PBYTE) pools;

        if (NPDescs != NULL)
        {
            memcpy(p, NPDescs, numNP * sizeof(POOL_DESC));
            p += numNP * sizeof(POOL_DESC);
        }

        if (PDescs != NULL)
        {
            memcpy(p, PDescs, numP * sizeof(POOL_DESC));
            p += numP * sizeof(POOL_DESC);
        }

        if (SDesc != NULL) 
        {
            POOL_DESC *tmp = (POOL_DESC *)p;
            tmp->kPoolDesc = SDesc;
            tmp->poolType = POOL_TYPE_SESSION;
        }

        *pNum = count;
    }

    if (NPDescs != NULL) free(NPDescs);
    if (PDescs != NULL) free(PDescs);
    
    return pools;
}



// Get the kernel pointer to the specified field of the current KPRCB 
WDBG_PTR GetPrcbFieldOffset(PCMD_CTX ctx, WDBG_PTR kprcb, const char *fieldName)
{
    if (kprcb == NULL)
        kprcb = GetExpression("@$prcb"); // use current if none specified
    return kprcb + GetStructField(ctx->pStructKPRCB, fieldName)->fieldOffset;
}

//
// fetch the specified lookaside from the list KPRCB
//
LOOKASIDE_DESC *GetKPRCBLookasides(PCMD_CTX ctx, const char *lookasideName, ULONG lookasideType, PULONG pNum)
{
    ULONG i;

    LOOKASIDE_DESC *pLookasides = (LOOKASIDE_DESC *) calloc(ctx->numKprcb, sizeof(LOOKASIDE_DESC));
    if (pLookasides == NULL)
        return NULL;

    for (i = 0; i < ctx->numKprcb; i++)
    {
        pLookasides[i].kLookasideDesc = GetPrcbFieldOffset(ctx, ctx->pKprcbs[i], lookasideName);
        pLookasides[i].poolType = lookasideType;
        pLookasides[i].numBuckets = ctx->lookasideBuckets;
        pLookasides[i].structSize = ctx->lookasideStructSize;
        pLookasides[i].processor = i;
    }
    
    *pNum = ctx->numKprcb;
    return pLookasides;
}

LOOKASIDE_DESC *GetNonPagedLookasides(PCMD_CTX ctx, PULONG pNum)
{
    return GetKPRCBLookasides(ctx, "PPNPagedLookasideList", POOL_TYPE_NONPAGED, pNum);
}

LOOKASIDE_DESC *GetNonPagedNxLookasides(PCMD_CTX ctx, PULONG pNum)
{
    PSYM_FIELD pField = GetStructField(ctx->pStructKPRCB, "PPNxPagedLookasideList");
    if (pField == NULL || pField->fieldTypeId == FIELD_NOT_PRESENT)
        return NULL;
    else
        return GetKPRCBLookasides(ctx, "PPNxPagedLookasideList", POOL_TYPE_NONPAGED, pNum);
}

LOOKASIDE_DESC *GetPagedLookasides(PCMD_CTX ctx, PULONG pNum)
{
    return GetKPRCBLookasides(ctx, "PPPagedLookasideList", POOL_TYPE_PAGED, pNum);
}

LOOKASIDE_DESC *GetSessionLookasides(PCMD_CTX ctx, PULONG pNum)
{
    WDBG_PTR kLookaside;
    WDBG_PTR kPool = GetSessionPool(ctx, &kLookaside);
    if (kPool)
    {
        LOOKASIDE_DESC *ret = (LOOKASIDE_DESC *) malloc(sizeof(LOOKASIDE_DESC));
        if (ret != NULL)
        {
            ret->kLookasideDesc = kLookaside;
            ret->structSize = ctx->sessionLookasideStructSize;
            ret->numBuckets = ctx->sessionLookasideBuckets;
            ret->poolType = POOL_TYPE_SESSION;
            ret->processor = 0;

            *pNum = 1;
        }
        return ret;
    }

    return NULL;
}


LOOKASIDE_DESC *GetAllLookasides(PCMD_CTX ctx, PULONG pNum)
{
    ULONG numNPNx = 0;
    ULONG numNP = 0;
    ULONG numP = 0;
    ULONG numS = 0;

    ULONG total = 0;

    LOOKASIDE_DESC *pNPNx = GetNonPagedNxLookasides(ctx, &numNPNx);
    LOOKASIDE_DESC *pNP = GetNonPagedLookasides(ctx, &numNP);
    LOOKASIDE_DESC *pP = GetPagedLookasides(ctx, &numP);
    LOOKASIDE_DESC *pS = GetSessionLookasides(ctx, &numS);
    LOOKASIDE_DESC *ret;
    PBYTE p;

    if (pNPNx) total += numNPNx;
    if (pNP) total += numNP;
    if (pP) total += numP;
    if (pS) total += numS;

    if (total == 0)
        return NULL;

    ret = (LOOKASIDE_DESC *) calloc(total, sizeof(LOOKASIDE_DESC));
    if (ret != NULL)
    {
        p = (PBYTE) ret;

        if (pNPNx)
        {
            memcpy(p, pNPNx, numNPNx * sizeof(LOOKASIDE_DESC));
            p += numNPNx * sizeof(LOOKASIDE_DESC);
        }

        if (pNP)
        {
            memcpy(p, pNP, numNP * sizeof(LOOKASIDE_DESC));
            p += numNP * sizeof(LOOKASIDE_DESC);
        }

        if (pP)
        {
            memcpy(p, pP, numP * sizeof(LOOKASIDE_DESC));
            p += numP * sizeof(LOOKASIDE_DESC);
        }

        if (pS)
        {
            memcpy(p, pS, numS * sizeof(LOOKASIDE_DESC));
            p += numS * sizeof(LOOKASIDE_DESC);
        }

        *pNum = total;
    }

    return ret;
}



void PrintTabularChunkHeader(PCMD_CTX ctx, ULONG indentLevel)
{
    ULONG i;

    for (i = 0; i < indentLevel; i++) dprintf(" ");
    dprintf(" %-*s  A/F   BlockSize     PreviousSize  PoolIndex PoolType Tag\n", ctx->pointerLen*2, "Addr");
    for (i = 0; i < indentLevel; i++) dprintf(" ");
    dprintf(" ");
    for (i = 0; i < (ctx->pointerLen * 2); i++) dprintf("-");
    dprintf("-----------------------------------------------------------\n");
}

template <typename HeaderType>
HRESULT PrintPoolHeaderTempl(PCMD_CTX ctx, WDBG_PTR kPtrChunk, HeaderType *hdr, BOOL bTabular, BOOL bRaw, ULONG indentLevel, BOOL bHighlight)
{
    HRESULT ret = S_OK;
    char tag[5];
    DWORD raw[4];
    HeaderType tmpHdr;

    // if no header data, just read it here
    if (hdr == NULL)
    {
        if (FAILED(ret = ReadVirtual(ctx, kPtrChunk, &tmpHdr, sizeof(tmpHdr))))
            return ret;
        hdr = &tmpHdr;
    }

    GetPoolTag(hdr->PoolTag, tag);

    for (ULONG i = 0; i < indentLevel; i++)
        dprintf(" ");

    if (bHighlight)
        OutputDml(ctx, "<col fg=\"emphfg\">");

    if (bTabular)
    {
        if (bHighlight)
            dprintf("*");
        else
            dprintf(" ");

        dprintf("%p: %-5s %04X (%03X)    %04X (%03X)           %02X       %02X %4s",
            kPtrChunk,
            ((hdr->PoolType & ctx->poolInUse) ? "InUse" : "Free"),
            hdr->BlockSize * ctx->poolBlockSize,
            hdr->BlockSize,
            hdr->PreviousSize * ctx->poolBlockSize,
            hdr->PreviousSize,
            hdr->PoolIndex, 
            hdr->PoolType,
            tag);
    }
    else
    {
        dprintf("%p: size:%03X prev:%03X index:%02X type:%02X tag:%4s", 
            kPtrChunk,
            hdr->BlockSize * ctx->poolBlockSize,
            hdr->PreviousSize * ctx->poolBlockSize,
            hdr->PoolIndex,
            hdr->PoolType,
            tag);
    }

    if (bRaw)
    {
        if (FAILED(ret = ReadVirtual(ctx, kPtrChunk, &raw, sizeof(raw))))
        {
            dprintf("\n");
            return ret;
        }

        dprintf("  %08X %08X %08X %08X",
            raw[0], raw[1], raw[2], raw[3]);
    }

    if (bHighlight)
        OutputDml(ctx, "</col>");

    dprintf("\n");
    return ret;
}

HRESULT PrintPoolHeader(PCMD_CTX ctx, WDBG_PTR kPtrChunk, PVOID hdr, BOOL bTabular, BOOL bRaw, ULONG indentLevel, BOOL bHighlight)
{
    if (ctx->b64BitTarget)
        return PrintPoolHeaderTempl<POOL_HEADER64>(ctx, kPtrChunk, (POOL_HEADER64 *)hdr, bTabular, bRaw, indentLevel, bHighlight);
    else
        return PrintPoolHeaderTempl<POOL_HEADER32>(ctx, kPtrChunk, (POOL_HEADER32 *)hdr, bTabular, bRaw, indentLevel, bHighlight);
}


BOOL EnumPoolPage(PCMD_CTX ctx, WDBG_PTR kPtr, LPARAM lParam, ENUMPAGEPROC enumProc)
{
    ULONG offset = 0;
    BYTE page[PAGE_SIZE];
    WDBG_PTR kPtrPage = PAGE_ALIGN(kPtr);
    WDBG_PTR kChunk;
    ULONG BlockSize;

    // read the entire page
    if (FAILED(ReadVirtual(ctx, kPtrPage, page, PAGE_SIZE)))
        return FALSE;

    while (TRUE)
    {
        // get the BlockSize
        BlockSize = GetBlockSize(ctx, (page + offset));

        // kernel pointer to the pool chunk
        kChunk = PAGE_ALIGN(kPtr) + offset;

        // call the callback function
        if (!enumProc(ctx, kChunk, (page + offset), lParam))
        {
            // callback requested a stop
            return FALSE;
        }

        // advance the offset to the next chunk
        // check that we don't have a 0-sized blocksize, which would infinite loop
        if (BlockSize == 0)
        {
            dprintf("Invalid BlockSize(0) at %p. Is this a pool page?\n", kChunk);
            return FALSE;
        }

        offset += (BlockSize * ctx->poolBlockSize);

        // if on next page, terminate
        if (PAGE_ALIGN(kPtrPage + offset) != kPtrPage)
        {
            // done with this page, did the last chunk make sense?
            // should be at beginning of next page
            if ((kPtrPage + offset) != (kPtrPage + PAGE_SIZE))
            {
                dprintf("** Invalid BlockSize %03X for %p ??\n", BlockSize, kChunk);
            }

            break;
        }
    }

    return TRUE;
}


BOOL EnumListHeadsBucket(PCMD_CTX ctx, WDBG_PTR kPoolDesc, ULONG bucket, LPARAM lParam, ENUMLISTHEADSPROC enumProc)
{
    BOOL bRet = TRUE;

    WDBG_PTR head;
    WDBG_PTR Flink;
    WDBG_PTR tmp;
    ULONG count = 0;

    PSYM_FIELD field = GetStructField(ctx->pStructPoolDescriptor, "ListHeads");
    WDBG_PTR kListHeads = kPoolDesc + field->fieldOffset;
    WDBG_PTR kBucketPtr = kListHeads + (bucket * (2 * ctx->pointerLen));

    // read the initial Flink
    if (FAILED(ReadTargetPointer(ctx, kBucketPtr, &Flink)))
        return FALSE;

    head = kBucketPtr;

    if (Flink == head)
    {
        // empty
        bRet = enumProc(ctx, bucket, TRUE, NULL, count, lParam);
    }
    else
    {
        while (Flink != head)
        {
            WDBG_PTR kChunk = Flink - (2 * ctx->pointerLen);

            // call the enum proc
            bRet = enumProc(ctx, bucket, FALSE, kChunk, count++, lParam);

            // terminate enumeration on FALSE return from enum proc
            if (!bRet)
                break;

            // read the Flink
            if (FAILED(ReadTargetPointer(ctx, Flink, &tmp)))
                return FALSE;

            // go to next entry
            Flink = tmp;
        }
    }

    return bRet;
}


BOOL EnumListHeads(PCMD_CTX ctx, WDBG_PTR kPoolDesc, ULONG bucket, LPARAM lParam, ENUMLISTHEADSPROC enumProc)
{
    BOOL ret;
    ULONG i;

    if (bucket == BUCKET_ALL)
    {
        // dump all the buckets
        for (i = 0; i < ctx->listHeadsBuckets; i++)
        {
            ret = EnumListHeadsBucket(ctx, kPoolDesc, i, lParam, enumProc);
            // terminate enumeration on FALSE return
            if (!ret) 
                break;
        }
    }
    else
    {
        // if just one bucket, dump out this bucket
       ret = EnumListHeadsBucket(ctx, kPoolDesc, bucket, lParam, enumProc);
    }

    return ret;
}






BOOL EnumLookasideBucket(PCMD_CTX ctx, LOOKASIDE_DESC *pLookasideDesc, ULONG bucket, LPARAM lParam, ENUMLOOKASIDEPROC enumProc)
{
    BOOL bRet = TRUE;
    WDBG_PTR kPtrs[2], pCur, pNext;
    ULONG count = 0;
    WDBG_PTR kLookasideBucket;
    PSYM_FIELD listField;

    if (bucket >= pLookasideDesc->numBuckets)
        return FALSE;

    kLookasideBucket = pLookasideDesc->kLookasideDesc + (bucket * pLookasideDesc->structSize);
    listField = GetStructField(ctx->pStructGeneralLookasidePool, "SingleListHead");

    // read the list head
    if (FAILED(ReadTargetPointerArray(ctx, kLookasideBucket + listField->fieldOffset, kPtrs, _countof(kPtrs))))
        return FALSE;

    // 64-bit lookaside struct has the Next pointer at [1]
    if (ctx->b64BitTarget)
        pCur = kPtrs[1];
    else
        pCur = kPtrs[0];

    if (pCur == NULL)
    {
        // empty list
        bRet = enumProc(ctx, kLookasideBucket, bucket, TRUE, NULL, 0, lParam);
    }
    else
    {
        while (pCur != NULL)
        {
            // get the chunk pointer
            WDBG_PTR kChunk = pCur - (2 * ctx->pointerLen);

            // call the enum proc
            bRet = enumProc(ctx, kLookasideBucket, bucket, FALSE, kChunk, count, lParam);

            // read the next pointer
            if (FAILED(ReadTargetPointer(ctx, pCur, &pNext)))
                return FALSE;

            pCur = pNext; // go to next entry in the list
            count++;
        }
    }

    return bRet;
}



BOOL EnumLookasides(PCMD_CTX ctx, LOOKASIDE_DESC *pLookasideDesc, ULONG bucket, LPARAM lParam, ENUMLOOKASIDEPROC enumProc)
{
    BOOL bRet = FALSE;
    ULONG i;

    if (bucket == BUCKET_ALL)
    {
        // dump all the buckets
        for (i = 0; i < pLookasideDesc->numBuckets; i++)
        {
            bRet = EnumLookasideBucket(ctx, pLookasideDesc, i, lParam, enumProc);
            // terminate enumeration on FALSE return
            if (!bRet) 
                break;
        }
    }
    else
    {
        // if just one bucket, dump out this bucket
        if (bucket < pLookasideDesc->numBuckets)
            bRet = EnumLookasideBucket(ctx, pLookasideDesc, bucket, lParam, enumProc);
    }

    return bRet;
}


BOOL CALLBACK EnumProcPrintHeader(PCMD_CTX ctx, WDBG_PTR kChunk, PVOID pHeader, LPARAM lParam)
{
    PPRINT_OPTS pPrintOpts = (PPRINT_OPTS)lParam;
    BOOL bHighlight = FALSE;
    ULONG BlockSize;

    if (pPrintOpts->bHighlight)
    {
        BlockSize = GetBlockSize(ctx, pHeader);
        if ((pPrintOpts->kPtr >= kChunk) && (pPrintOpts->kPtr < (kChunk + (BlockSize * ctx->poolBlockSize))))
            bHighlight = TRUE;
    }

    // if tabular and first chunk, print the header
    if (pPrintOpts->bTabular && pPrintOpts->count == 0)
        PrintTabularChunkHeader(ctx, pPrintOpts->indentLevel);

    PrintPoolHeader(ctx, kChunk, pHeader, pPrintOpts->bTabular, pPrintOpts->bRaw, pPrintOpts->indentLevel, bHighlight);

    pPrintOpts->count++;
    return TRUE;
}



BOOL CALLBACK EnumPoolProcFindHeader(PCMD_CTX ctx, WDBG_PTR kChunk, PVOID pHeader, LPARAM lParam)
{
    PFIND_PTR find = (PFIND_PTR) lParam;
    ULONG BlockSize = GetBlockSize(ctx, pHeader);

    if (PTR_IN_CHUNK(ctx, find->kPointer, kChunk, BlockSize))
    {
        find->kChunk = kChunk;
        return FALSE; // stop enumerating
    }

    return TRUE;
}

WDBG_PTR FindPoolHeader(PCMD_CTX ctx, WDBG_PTR kPtr)
{
    FIND_PTR find = {0};
    find.kChunk = NULL;
    find.kPointer = kPtr;

    EnumPoolPage(ctx, kPtr, (LPARAM)&find, EnumPoolProcFindHeader);

    return find.kChunk;
}

BOOL CALLBACK EnumListHeadsProcFindChunk(PCMD_CTX ctx, ULONG bucket, BOOL bEmpty, WDBG_PTR kChunk, ULONG count, LPARAM lParam)
{
    BYTE header[8];
    PFIND_PTR find = (PFIND_PTR)lParam;

    if (!bEmpty)
    {
        if (FAILED(ReadVirtual(ctx, kChunk, header, sizeof(header))))
            return FALSE;

        ULONG BlockSize = GetBlockSize(ctx, header);

        if (PTR_IN_CHUNK(ctx, find->kPointer, kChunk, BlockSize))
        {
            find->kChunk = kChunk;
            find->bucket = bucket;
            find->count = count;
            return FALSE;
        }
    }

    return TRUE;
}

BOOL CALLBACK EnumLookasideProcFindChunk(PCMD_CTX ctx, WDBG_PTR kLookasideBucket, ULONG bucket, BOOL bEmpty, WDBG_PTR kChunk, ULONG count, LPARAM lParam)
{
    BYTE header[8];
    PFIND_PTR find = (PFIND_PTR)lParam;

    if (!bEmpty)
    {
        if (FAILED(ReadVirtual(ctx, kChunk, header, sizeof(header))))
            return FALSE;

        ULONG BlockSize = GetBlockSize(ctx, header);

        // is the pointer in this chunk?
        if (PTR_IN_CHUNK(ctx, find->kPointer, kChunk, BlockSize))
        {
            find->kChunk = kChunk;
            find->bucket = bucket;
            find->count = count;
            find->kDesc = kLookasideBucket;
            return FALSE;
        }
    }

    return TRUE;
}

BOOL FindChunkInListHeads(PCMD_CTX ctx, WDBG_PTR kPoolDesc, WDBG_PTR kPtr, PFIND_PTR pFind)
{
    pFind->kPointer = kPtr;

    if (EnumListHeads(ctx, kPoolDesc, BUCKET_ALL, (LPARAM)pFind, EnumListHeadsProcFindChunk) == FALSE)
    {
        // chunk found
        return TRUE;
    }
    else
    {
        // didn't find it
        return FALSE;
    }
}

BOOL FindChunkInLookasideBucket(PCMD_CTX ctx, WDBG_PTR kLookasideBucket, WDBG_PTR kPtr, ULONG bucket, WDBG_PTR kLookaside, const char *lookasideType)
{
    BOOL bFound = FALSE;
    WDBG_PTR kPtrs[2], pCur, pNext;
    ULONG count = 0;
    ULONG BlockSize;
    BYTE header[POOL_HEADER_SIZE];
    PSYM_FIELD listField = GetStructField(ctx->pStructGeneralLookasidePool, "SingleListHead");

    // read the list head
    if (FAILED(ReadTargetPointerArray(ctx, kLookasideBucket + listField->fieldOffset, kPtrs, _countof(kPtrs))))
        return FALSE;

    if (ctx->b64BitTarget)
        pCur = kPtrs[1];
    else
        pCur = kPtrs[0];

    while (pCur != NULL)
    {
        // read the pool header
        WDBG_PTR kChunk = pCur - (2 * ctx->pointerLen);
        if (FAILED(ReadVirtual(ctx, kChunk, &header, sizeof(header))))
            break;

        BlockSize = GetBlockSize(ctx, header);

        // is the pointer in this chunk?
        if (PTR_IN_CHUNK(ctx, kPtr, kChunk, BlockSize))
        {
            const char *name = GetLookasideType(ctx, kLookaside);
            dprintf("%p: #%u in <link cmd=\"!poolinfo -s -l -b %X -t %s %p\">%s Lookaside[%02X]</link> (size=%03X) at %p\n", 
                kPtr, count + 1, 
                BUCKET_TO_SIZE(bucket, ctx->poolBlockSize),
                lookasideType, kLookaside,
                (name ? name : "Unknown"),
                bucket, 
                BUCKET_TO_SIZE(bucket, ctx->poolBlockSize), 
                kLookasideBucket);
            bFound = TRUE;
            break;
        }

        // read the next pointer
        if (FAILED(ReadTargetPointer(ctx, pCur, &pNext)))
            break;

        pCur = pNext; // go to next entry in the list
        count++;
    }


    return bFound;
}

BOOL FindFreeChunk(PCMD_CTX ctx, WDBG_PTR kPtr)
{
    BOOL bFound = FALSE;
    POOL_DESC *pPoolDescs;
    LOOKASIDE_DESC *pLookasideDescs;
    DWORD numDescs = 0;
    DWORD i;
    FIND_PTR find = {0};
    const char *poolName;
    ULONG poolIndex = 0;

    // chunk is free, this is harder as the PoolType will be 0
    // loop through all the pool descriptors and their ListHeads (yuck)
    // if not there, try the lookaside
    // if not there, ragequit

    pPoolDescs = GetAllPoolDescriptors(ctx, &numDescs);
    if (pPoolDescs == NULL)
        return FALSE;

    DEBUGPRINT("Searching ListHeads\n");

    // optimization? if PoolType == 0 search ListHeads?
    for (i = 0; i < numDescs; i++)
    {
        DEBUGPRINT("Searching in pool %p\n", pPoolDescs[i].kPoolDesc);

        if (FindChunkInListHeads(ctx, pPoolDescs[i].kPoolDesc, kPtr, &find))
        {
            poolName = GetPoolType(ctx, pPoolDescs[i].kPoolDesc, &poolIndex);
            OutputDml(ctx,
                "%p: #%u in <link cmd=\"!poolinfo -f -b %X -v %p\">ListHeads[%02X]</link> in pool %s[%u] (%p)\n", 
                kPtr,
                find.count + 1,
                BUCKET_TO_SIZE(find.bucket,  ctx->poolBlockSize),
                pPoolDescs[i].kPoolDesc,
                find.bucket,
                poolName ? poolName : "Unknown",
                poolIndex,
                pPoolDescs[i].kPoolDesc);

            bFound = TRUE;
            break;
        }
    }

    free(pPoolDescs);
    pPoolDescs = NULL;

    if (!bFound)
    {
        DEBUGPRINT("Searching Lookasides\n");

        pLookasideDescs = GetAllLookasides(ctx, &numDescs);
        if (pLookasideDescs == NULL)
            return FALSE;

        // optimization? if PoolType != 0 search Lookaside of correct pool...

        FIND_PTR find = {0};
        find.kPointer = kPtr;
        
        for (i = 0; i < numDescs; i++)
        {
            DEBUGPRINT("Searching lookaside %p\n", pLookasideDescs[i].kLookasideDesc);

            if (!EnumLookasides(ctx, &pLookasideDescs[i], BUCKET_ALL, (LPARAM)&find, EnumLookasideProcFindChunk))
            {
                // enum returned false, we found the pointer

                const char *lookasideInfoType = GetLookasideInfoType(pLookasideDescs[i].poolType);
                const char *lookasideName = GetLookasideType(ctx, find.kDesc);

                OutputDml(ctx,
                    "%p: #%u in <link cmd=\"!poolinfo -s -l -b %X -t %s %p\">%s Lookaside[%02X]</link> (size=%03X) at %p\n", 
                    kPtr,
                    find.count + 1, 
                    BUCKET_TO_SIZE(find.bucket, ctx->poolBlockSize),
                    lookasideInfoType,
                    pLookasideDescs->kLookasideDesc,
                    (lookasideName ? lookasideName : "Unknown"),
                    find.bucket, 
                    BUCKET_TO_SIZE(find.bucket, ctx->poolBlockSize), 
                    find.kDesc);

                bFound = TRUE;
                break;
            }
        }
        
        free(pLookasideDescs);
    }

    return bFound;
}

HRESULT PrintExtendedChunkInfo(PCMD_CTX ctx, WDBG_PTR kPtr)
{
    HRESULT ret = S_OK;
    ULONG PoolType;
    ULONG PoolIndex;
    BYTE header[POOL_HEADER_SIZE];
    POOL_DESC *poolDescs;
    DWORD numDescs = 0;
    WDBG_PTR kLookasideSession;
    BOOL bFound = FALSE;

    // find the pool that this chunk is in.
    if (FAILED(ret = ReadVirtual(ctx, kPtr, header, sizeof(header))))
        return ret;

    if (ctx->b64BitTarget)
    {
        PoolType = ((PPOOL_HEADER64)header)->PoolType;
        PoolIndex = ((PPOOL_HEADER64)header)->PoolIndex;
    }
    else
    {
        PoolType = ((PPOOL_HEADER32)header)->PoolType;
        PoolIndex = ((PPOOL_HEADER32)header)->PoolIndex;
    }

    // is the chunk in-use? if so, we can get the pooltype
    if (PoolType & ctx->poolInUse)
    {
        if ((PoolType & PagedPoolSession) == PagedPoolSession)
        {
            poolDescs = (POOL_DESC *) malloc(sizeof(WDBG_PTR) * 1);
            if (poolDescs == NULL)
                return E_OUTOFMEMORY;
            numDescs = 1;
            poolDescs[0].kPoolDesc = GetSessionPool(ctx, &kLookasideSession);
            poolDescs[0].poolType = POOL_TYPE_SESSION;
        }
        else if ((PoolType & PagedPool) == PagedPool)
        {
            poolDescs = GetPagedPools(ctx, &numDescs);
        }
        else
        {
            poolDescs = GetNonPagedPools(ctx, &numDescs);
        }

        // does the PoolIndex make sense?
        if (PoolIndex >= numDescs)
        {
            dprintf("PoolIndex %02X is too large for the %s\n",
                PoolIndex, PoolTypeName((POOL_TYPE)PoolType));
            ret = E_FAIL;
        }
        else
        {
            const char *poolName = GetPoolType(ctx, poolDescs[PoolIndex].kPoolDesc, NULL);
            dprintf("%p is InUse on %s[%u], page: ", kPtr, (poolName ? poolName : "Unknown"), PoolIndex);
            OutputDml(ctx, "<link cmd=\"!poolpage %p\">%p</link>\n", PAGE_ALIGN(kPtr), PAGE_ALIGN(kPtr));
        }
    }
    else
    {
        dprintf("chunk appears to be free, searching descriptors...\n");

        bFound = FindFreeChunk(ctx, kPtr);

        if (!bFound)
        {
            dprintf("Failed to locate %p\n", kPtr);
        }
    }

    return ret;
}

