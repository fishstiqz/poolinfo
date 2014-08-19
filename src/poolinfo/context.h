#pragma once

// windbg pointer is 64-bit
typedef ULONG64 WDBG_PTR, *PWDBG_PTR;

//
// abstract structure representation
//
typedef struct _SYM_FIELD
{
    const char *    fieldName;
    BOOL            fieldOptional;
    ULONG           fieldOffset;
    ULONG           fieldTypeId;
    ULONG           fieldSize;
    PVOID           fieldData;
}
SYM_FIELD, *PSYM_FIELD;

#define DECL_FIELD(name)        { #name, FALSE, 0, 0, 0, NULL }
#define DECL_FIELD_OPT(name)    { #name, TRUE , 0, 0, 0, NULL } 
#define END_FIELD               {  NULL, FALSE, 0, 0, 0, NULL }

#define FIELD_NOT_PRESENT       ((ULONG)-1)


//
// command context
//
typedef struct _CMD_CTX
{
    // os versioning stuff
    ULONG                   osMajor;
    ULONG                   osMinor;
    BOOL                    b64BitTarget;
    ULONG                   pointerLen;
    ULONG                   poolBlockSize;
    ULONG                   listHeadsBuckets;
    ULONG                   poolInUse;
    ULONG                   lookasideStructSize;
    ULONG                   lookasideBuckets;
    ULONG                   sessionLookasideStructSize;
    ULONG                   sessionLookasideBuckets;

    WDBG_PTR  *             pKprcbs;
    ULONG                   numKprcb;

    // debugging interfaces
    PDEBUG_CLIENT4          pDebugClient;
    PDEBUG_SYMBOLS3         pDebugSymbols;
    PDEBUG_CONTROL4         pDebugControl;
    PDEBUG_DATA_SPACES3     pDebugDataSpaces;
    PDEBUG_SYSTEM_OBJECTS4  pDebugSystemObjects;

    // structure defs
    PSYM_FIELD              pStructPoolDescriptor;
    PSYM_FIELD              pStructEprocess;
    PSYM_FIELD              pStructMmSessionSpace;
    PSYM_FIELD              pStructKPRCB;
    PSYM_FIELD              pStructGeneralLookasidePool;

}
CMD_CTX, *PCMD_CTX;

//
// function prototypes
//
WDBG_PTR *GetAllKprcb(PDEBUG_DATA_SPACES3 pDebugDataSpaces, PULONG pNum);
HRESULT ReleaseCmdCtx(PCMD_CTX ctx);
HRESULT InitCmdCtx(PCMD_CTX ctx, PDEBUG_CLIENT4 pClient);


