#pragma once


//
// structs resolved with ResolveAllStructs
//
extern SYM_FIELD g_StructPoolDescriptor[];
extern SYM_FIELD g_StructEprocess[];
extern SYM_FIELD g_StructMmSessionSpace[];
extern SYM_FIELD g_StructKPRCB[];
extern SYM_FIELD g_StructGeneralLookasidePool[];



//
// functions
//
extern HRESULT ResolveStruct(PDEBUG_SYMBOLS3 pDebugSymbols, const char *moduleName, const char *structName, PSYM_FIELD symFields);
extern void DumpResolvedStruct(PSYM_FIELD pStruct);
extern HRESULT ResolveAllStructs(PCMD_CTX ctx);
extern PSYM_FIELD CloneStruct(PSYM_FIELD pBaseStruct);
extern ULONG GetReadStructLength(PSYM_FIELD pStruct);
extern HRESULT ReadVirtualStruct(PCMD_CTX ctx, WDBG_PTR addr, PSYM_FIELD pStruct, PVOID *pStructData);
PSYM_FIELD GetStructField(PSYM_FIELD pStruct, const char *name);
WDBG_PTR GetStructPointer(PCMD_CTX ctx, PSYM_FIELD pStruct, const char *name);

// accessor template
template <typename Type>
Type GetStructValTempl(PSYM_FIELD pStruct, const char *name, Type defVal);

// accessors
#define DECLARE_STRUCT_ACCESSOR(type) \
    extern type GS_ ## type (PSYM_FIELD pStruct, const char *name);



DECLARE_STRUCT_ACCESSOR(ULONG);
DECLARE_STRUCT_ACCESSOR(DWORD);
DECLARE_STRUCT_ACCESSOR(WORD);
DECLARE_STRUCT_ACCESSOR(BYTE);
DECLARE_STRUCT_ACCESSOR(ULONG32);
DECLARE_STRUCT_ACCESSOR(LONG32);

#undef DECLARE_STRUCT_ACCESSOR
