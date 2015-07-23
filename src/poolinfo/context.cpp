#include "stdafx.h"



WDBG_PTR *GetAllKprcb(PDEBUG_DATA_SPACES3 pDebugDataSpaces, PULONG pNum)
{
    HRESULT res;
    ULONG i, numProc = 0;
    WDBG_PTR kKprcb;
    WDBG_PTR *kKprcbs = NULL;
    ULONG dataSize = 0;

    // first get the number of KPRCB
    while (TRUE)
    {
        res = pDebugDataSpaces->ReadProcessorSystemData(numProc, DEBUG_DATA_KPRCB_OFFSET, &kKprcb, sizeof(kKprcb), &dataSize);
        if (FAILED(res))
            break;

        numProc++;
    }

    // now put them all in a list
    if (numProc > 0)
    {
        kKprcbs = (WDBG_PTR *) calloc(numProc, sizeof(WDBG_PTR));
        for (i = 0; i < numProc; i++)
        {
            res = pDebugDataSpaces->ReadProcessorSystemData(numProc, DEBUG_DATA_KPRCB_OFFSET, &kKprcb, sizeof(kKprcb), &dataSize);
            kKprcbs[i] = kKprcb;
        }
    }

    *pNum = numProc;
    return kKprcbs;
}

HRESULT ReleaseCmdCtx(PCMD_CTX ctx)
{
#define _RELEASE_IFACE(p) \
    if (p) { (p)->Release(); (p) = NULL; }

    _RELEASE_IFACE(ctx->pDebugSymbols);
    _RELEASE_IFACE(ctx->pDebugDataSpaces);
    _RELEASE_IFACE(ctx->pDebugControl);
    _RELEASE_IFACE(ctx->pDebugSystemObjects);

#undef _RELEASE_IFACE

#define _FREE_PTR(s) \
    if (s) { free(s); (s) = NULL; }

    _FREE_PTR(ctx->pStructPoolDescriptor);
    _FREE_PTR(ctx->pStructEprocess);
    _FREE_PTR(ctx->pStructMmSessionSpace);
    _FREE_PTR(ctx->pStructKPRCB);
    _FREE_PTR(ctx->pStructGeneralLookasidePool);

    _FREE_PTR(ctx->pKprcbs);

#undef _FREE_STRUCT

    return S_OK;
}

HRESULT InitCmdCtx(PCMD_CTX ctx, PDEBUG_CLIENT4 pClient)
{
    HRESULT ret;
    ULONG dbgClass, dbgQualifier;

    memset(ctx, 0, sizeof(CMD_CTX));
    
    ctx->pDebugClient = pClient;

#define _CREATE_IFACE(type, ptr) \
    if ((ret = DebugCreate(__uuidof(type), (PVOID *)(ptr))) != S_OK) { \
        dprintf("failed to create " #type "\n"); \
        ReleaseCmdCtx(ctx); \
        return ret; \
    }

    _CREATE_IFACE(IDebugSymbols3,        &ctx->pDebugSymbols)
    _CREATE_IFACE(IDebugDataSpaces3,     &ctx->pDebugDataSpaces)
    _CREATE_IFACE(IDebugControl4,        &ctx->pDebugControl)
    _CREATE_IFACE(IDebugSystemObjects4,  &ctx->pDebugSystemObjects)

#undef _CREATE_IFACE

    // is this in the kernel?
    if (FAILED(ret = ctx->pDebugControl->GetDebuggeeType(&dbgClass, &dbgQualifier)))
    {
        dprintf("GetDebuggeeType failed\n");
        ReleaseCmdCtx(ctx);
        return ret;
    }

    if (dbgClass != DEBUG_CLASS_KERNEL)
    {
        dprintf("This extension is for kernel-mode only\n");
        ReleaseCmdCtx(ctx);
        return ret;
    }

    // whats the platform?
    ULONG platformId;
    ret = ctx->pDebugControl->GetSystemVersionValues(&platformId, &ctx->osMajor, &ctx->osMinor, &ctx->osFree, &ctx->osBuild);
    if (FAILED(ret))
    {
        dprintf("GetSystemVersionValues failed");
        ReleaseCmdCtx(ctx);
        return ret;
    }

    DEBUGPRINT("OS Detected: %d.%d.%d Free=%u\n", ctx->osMajor, ctx->osMinor, ctx->osBuild, (ctx->osFree == 0xF));
    
    ctx->poolInUse = (ctx->osMajor <= 5 ? POOL_INUSE_XP : POOL_INUSE);

    if (ctx->pDebugControl->IsPointer64Bit() == S_OK)
    {
        ctx->b64BitTarget = TRUE;
        ctx->pointerLen = 8;
        ctx->poolBlockSize = POOL_BLOCK_SIZE64;
        ctx->listHeadsBuckets = 0xff + 1;
        ctx->sessionLookasideBuckets = 0x15;
    }
    else
    {
        ctx->b64BitTarget = FALSE;
        ctx->pointerLen = 4;
        ctx->poolBlockSize = POOL_BLOCK_SIZE32;
        ctx->listHeadsBuckets = 0x1ff + 1;
        ctx->sessionLookasideBuckets = 0x19;
    }


    if (FAILED(ret = ResolveAllStructs(ctx)))
    {
        dprintf("Failed to resolve the required structures. Are symbols loaded?\n");
        ReleaseCmdCtx(ctx);
        return ret;
    }

    // get sizes for lookasides
    PSYM_FIELD lastField = GetStructField(ctx->pStructGeneralLookasidePool, "Future");
    ctx->lookasideStructSize = lastField->fieldOffset + lastField->fieldSize;
    ctx->lookasideBuckets = POOL_LOOKASIDE_BUCKETS;
    ctx->sessionLookasideStructSize = SESSION_LOOKASIDE_SIZE;

    // get all the KPRCB
    ctx->pKprcbs = GetAllKprcb(ctx->pDebugDataSpaces, &ctx->numKprcb);
    if (ctx->pKprcbs == NULL)
    {
        ReleaseCmdCtx(ctx);
        return E_FAIL;
    }

    return ret;
}

