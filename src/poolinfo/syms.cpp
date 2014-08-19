#include "stdafx.h"


// nt!_POOL_DESCRIPTOR
SYM_FIELD g_StructPoolDescriptor[] =
{
    DECL_FIELD(PoolType),
    DECL_FIELD(PoolIndex),
    DECL_FIELD(PendingFreeDepth),
    DECL_FIELD(RunningAllocs),
    DECL_FIELD(RunningDeAllocs),
    DECL_FIELD(TotalBigPages),
    DECL_FIELD(ThreadsProcessingDeferrals),
    DECL_FIELD(TotalBytes),
    DECL_FIELD(TotalPages),
    DECL_FIELD(ListHeads),
    END_FIELD
};

// nt!_EPROCESS
SYM_FIELD g_StructEprocess[] = 
{
    DECL_FIELD(Session),
    END_FIELD
};

//  nt!_MM_SESSION_SPACE
SYM_FIELD g_StructMmSessionSpace[] =
{
    DECL_FIELD(SessionId),
    DECL_FIELD(Lookaside),
    DECL_FIELD(PagedPool),
    END_FIELD
};

// nt!_KPRCB
SYM_FIELD g_StructKPRCB[] =
{
    DECL_FIELD(PPLookasideList),
    DECL_FIELD_OPT(PPNxPagedLookasideList),
    DECL_FIELD(PPNPagedLookasideList),
    DECL_FIELD(PPPagedLookasideList),
    END_FIELD
};

// nt!_GENERAL_LOOKASIDE_POOL
SYM_FIELD g_StructGeneralLookasidePool[] =
{
    DECL_FIELD(SingleListHead),
    DECL_FIELD(Depth),
    DECL_FIELD(MaximumDepth),
    DECL_FIELD(AllocateMisses),
    DECL_FIELD(AllocateHits),
    DECL_FIELD(TotalFrees),
    DECL_FIELD(FreeMisses),
    DECL_FIELD(FreeHits),
    DECL_FIELD(Type),
    DECL_FIELD(Size),
    DECL_FIELD(Tag),
    DECL_FIELD(Future),         // the last entry, for size calculation
    END_FIELD
};



HRESULT ResolveStruct(PDEBUG_SYMBOLS3 pDebugSymbols, const char *moduleName, const char *structName, PSYM_FIELD symFields)
{
    HRESULT ret = S_OK;
    ULONG64 modBase = 0;
    PSYM_FIELD field = symFields;
    ULONG typeId;
    ULONG fieldOffset;
    ULONG fieldTypeId;
    ULONG fieldSize;

    if (moduleName != NULL)
    {
        if (FAILED(ret = pDebugSymbols->GetModuleByModuleName(moduleName, 0, NULL, &modBase)))
        {
            dprintf("failed to find module: %s\n", moduleName);
            return ret;
        }
    }
    else
    {
        if (FAILED(ret = pDebugSymbols->GetSymbolModule(structName, &modBase)))
        {
            dprintf("failed to find module for symbol: %s\n", structName);
            return ret;
        }
    }

    if (FAILED(ret = pDebugSymbols->GetTypeId(modBase, structName, &typeId)))
    {
        dprintf("failed to find structName: %s, modBase: %p\n", structName, modBase);
        return ret;
    }

    while (field->fieldName != NULL)
    {
        if (FAILED(ret = pDebugSymbols->GetFieldTypeAndOffset(modBase, typeId, field->fieldName, &fieldTypeId, &fieldOffset)))
        {
            if (!field->fieldOptional)
            {
                dprintf("failed to resolve field for %s\n", field->fieldName);
                return ret;
            }
            else
            {
                field->fieldOffset = 0;
                field->fieldTypeId = FIELD_NOT_PRESENT;   
                field->fieldSize = 0;
                field++;
                continue;
            }
        }

        if (FAILED(ret = pDebugSymbols->GetTypeSize(modBase, fieldTypeId, &fieldSize)))
        {
            dprintf("failed to resolve field size for %s\n", field->fieldName);
            return ret;
        }

        field->fieldOffset = fieldOffset;
        field->fieldTypeId = fieldTypeId;   
        field->fieldSize = fieldSize;
        field++;
    }

    return ret;
}

void DumpResolvedStruct(PSYM_FIELD pStruct)
{
    PSYM_FIELD pField = pStruct;
    while (pField->fieldName != NULL)
    {
        dprintf("%-30s: offset=%08X, type=%08X, size=%08X\n",
            pField->fieldName, pField->fieldOffset, pField->fieldTypeId, pField->fieldSize);
        pField++;
    }
}


// clone a struct
PSYM_FIELD CloneStruct(PSYM_FIELD pBaseStruct)
{
    ULONG len = 0;
    PSYM_FIELD pClone;
    PSYM_FIELD pField = pBaseStruct;

    // find the last field to determine the size
    while (pField->fieldName != NULL)
    {
        len++;
        pField++;
    }

    // shallow copy the struct
    pClone = (PSYM_FIELD) malloc((len + 1) * sizeof(SYM_FIELD));
    if (pClone != NULL)
        memcpy(pClone, pBaseStruct, (len + 1) * sizeof(SYM_FIELD));

    return pClone;
}

HRESULT ResolveAllStructs(PCMD_CTX ctx)
{
    HRESULT ret = S_OK;

#define _RESOLVE(name, symStruct, ctxStruct) \
    ctxStruct = CloneStruct(symStruct); \
    if (ctxStruct == NULL) { \
        return E_OUTOFMEMORY; \
    } \
    if (FAILED(ret = ResolveStruct(ctx->pDebugSymbols, NULL, (name), (ctxStruct)))) { \
        dprintf("failed to resolve %s\n", (name)); \
        return ret; \
    } \
    //DEBUGPRINT("%s\n", (name)); \
    //if (g_bDebug) DumpResolvedStruct(symStruct)

    _RESOLVE("nt!_POOL_DESCRIPTOR",         g_StructPoolDescriptor,         ctx->pStructPoolDescriptor);
    _RESOLVE("nt!_EPROCESS",                g_StructEprocess,               ctx->pStructEprocess);
    _RESOLVE("nt!_MM_SESSION_SPACE",        g_StructMmSessionSpace,         ctx->pStructMmSessionSpace);
    _RESOLVE("nt!_KPRCB",                   g_StructKPRCB,                  ctx->pStructKPRCB);
    _RESOLVE("nt!_GENERAL_LOOKASIDE_POOL",  g_StructGeneralLookasidePool,   ctx->pStructGeneralLookasidePool);

    return ret;
}



//
// returns the amount we need to read to get 
// the entire structure as defind by the PSYM_FIELD.
// this is NOT the actual struct length, just what we
// care about
//
ULONG GetReadStructLength(PSYM_FIELD pStruct)
{
    ULONG length = 0;
    ULONG offset = 0;

    PSYM_FIELD pField = pStruct;
    while (pField->fieldName != NULL)
    {
        if (pField->fieldOffset > offset)
        {
            offset = pField->fieldOffset;
            length = offset + pField->fieldSize;
        }
        
        pField++;
    }

    return length;
}

//
// reads a struct from a virtual address
//  the struct data is placed in pStructData and should be freed when done
//  the pStruct should be a cloned structure which will have its fieldData
//   pointers updated to point to the the appropriate location in pStructData
//
HRESULT ReadVirtualStruct(PCMD_CTX ctx, WDBG_PTR addr, PSYM_FIELD pStruct, PVOID *pStructData)
{
    HRESULT ret;
    ULONG structLen = GetReadStructLength(pStruct);

    PBYTE buf = (PBYTE) malloc(structLen);
    if (buf == NULL)
        return E_OUTOFMEMORY;

    if (SUCCEEDED(ret = ReadVirtual(ctx, addr, buf, structLen)))
    {
        // assign the fieldData pointer to the proper location in buf
        PSYM_FIELD pField = pStruct;
        while (pField->fieldName != NULL)
        {
            pField->fieldData = buf + pField->fieldOffset;
            pField++;
        }

        *pStructData = buf;
    }

    return ret;
}

PSYM_FIELD GetStructField(PSYM_FIELD pStruct, const char *name)
{
    PSYM_FIELD pField = pStruct;
    while (pField->fieldName != NULL)
    {
        if (strcmp(pField->fieldName, name) == 0)
            return pField;

        pField++;
    }

    return NULL;
}

WDBG_PTR GetStructPointer(PCMD_CTX ctx, PSYM_FIELD pStruct, const char *name)
{
    WDBG_PTR val = 0;
    PSYM_FIELD pField = GetStructField(pStruct, name);

    if (pField != NULL)
    {
        if (ctx->pointerLen == 8)
        {
            ULONG64 ptr = *((ULONG64 *)(pField->fieldData));
            val = ptr;
        }
        else
        {
            ULONG32 ptr = *((ULONG32 *)(pField->fieldData));
            val = SIGN_EXTEND32(ptr);
        }
    }

    return val;
}

template <typename Type>
Type GetStructValTempl(PSYM_FIELD pStruct, const char *name, Type defVal)
{
    Type val = defVal;
    PSYM_FIELD pField = GetStructField(pStruct, name);
    if (pField != NULL)
        val = *((Type *)(pField->fieldData));
    return val;
}

#define DECLARE_STRUCT_ACCESSOR(type, defVal) \
    type GS_ ## type (PSYM_FIELD pStruct, const char *name) { \
        return GetStructValTempl< type >(pStruct, name, (defVal)); \
    }


DECLARE_STRUCT_ACCESSOR(ULONG, 0);
DECLARE_STRUCT_ACCESSOR(DWORD, 0);
DECLARE_STRUCT_ACCESSOR(WORD, 0);
DECLARE_STRUCT_ACCESSOR(BYTE, 0);
DECLARE_STRUCT_ACCESSOR(ULONG32, 0);
DECLARE_STRUCT_ACCESSOR(LONG32, 0);

#undef DECLARE_STRUCT_ACCESSOR
