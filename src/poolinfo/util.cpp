#include "stdafx.h"

#pragma warning(disable: 4996)

//
// mostly taken from http://alter.org.ua/docs/win/args/
// thanks to Alter
//
PCHAR* CommandLineToArgvA(PCHAR CmdLine, int* _argc)
{
    PCHAR* argv;
    PCHAR  _argv;
    size_t  len;
    ULONG   argc;
    CHAR   a;
    size_t  i, j;

    BOOLEAN  in_QM;
    BOOLEAN  in_TEXT;
    BOOLEAN  in_SPACE;

    len = strlen(CmdLine);
    i = ((len+2)/2)*sizeof(PVOID) + sizeof(PVOID);

    //argv = (PCHAR*)GlobalAlloc(GMEM_FIXED, i + (len+2)*sizeof(CHAR));
    argv = (PCHAR*)malloc(i + (len+2)*sizeof(CHAR));

    _argv = (PCHAR)(((PUCHAR)argv)+i);

    argc = 0;
    argv[argc] = _argv;
    in_QM = FALSE;
    in_TEXT = FALSE;
    in_SPACE = TRUE;
    i = 0;
    j = 0;

    while( a = CmdLine[i] ) {
        if(in_QM) {
            if(a == '\"') {
                in_QM = FALSE;
            } else {
                _argv[j] = a;
                j++;
            }
        } else {
            switch(a) {
            case '\"':
                in_QM = TRUE;
                in_TEXT = TRUE;
                if(in_SPACE) {
                    argv[argc] = _argv+j;
                    argc++;
                }
                in_SPACE = FALSE;
                break;
            case ' ':
            case '\t':
            case '\n':
            case '\r':
                if(in_TEXT) {
                    _argv[j] = '\0';
                    j++;
                }
                in_TEXT = FALSE;
                in_SPACE = TRUE;
                break;
            default:
                in_TEXT = TRUE;
                if(in_SPACE) {
                    argv[argc] = _argv+j;
                    argc++;
                }
                _argv[j] = a;
                j++;
                in_SPACE = FALSE;
                break;
            }
        }
        i++;
    }
    _argv[j] = '\0';
    argv[argc] = NULL;

    (*_argc) = argc;
    return argv;
}

char **MakeCmdline(PCSTR argv0, PCSTR args, int* pargc)
{
    char **argv = NULL;

    // make a temporary full command line
    size_t len = strlen(argv0) + strlen(args) + 2;
    char *cmdline = (char *) malloc(len);
    if (cmdline == NULL)
        return NULL;

    strcpy(cmdline, argv0);
    strcat(cmdline, " ");
    strcat(cmdline, args);

    argv = CommandLineToArgvA(cmdline, pargc);
    free(cmdline);
    return argv;
}

HRESULT OutputDmlBase(PDEBUG_CONTROL4 pDebugControl, const char *fmt, va_list ap)
{
    return pDebugControl->ControlledOutputVaList(DEBUG_OUTCTL_AMBIENT_DML, DEBUG_OUTPUT_NORMAL, fmt, ap);
}

HRESULT OutputDml(PCMD_CTX ctx, const char *fmt, ...)
{
    HRESULT ret;
    va_list ap;

    va_start(ap, fmt);
    ret = OutputDmlBase(ctx->pDebugControl,  fmt, ap);
    //ret = ctx->pDebugControl->ControlledOutputVaList(DEBUG_OUTCTL_AMBIENT_DML, DEBUG_OUTPUT_NORMAL, fmt, ap);
    va_end(ap);
    return ret;
}

HRESULT ReadVirtual(PCMD_CTX ctx, WDBG_PTR src, PVOID dst, ULONG len)
{
    HRESULT ret;
    ULONG numRead = 0;
    WDBG_PTR ptrToRead = src;

    if (ctx->b64BitTarget)
        ret = ctx->pDebugDataSpaces->ReadVirtual(src, dst, len, &numRead);
    else
        ret = ctx->pDebugDataSpaces->ReadVirtual(SIGN_EXTEND32(src), dst, len, &numRead);

    if (ret == S_OK)
    {
        if (numRead != len)
        {
            dprintf("poolinfo: ReadVirtual returned only %u bytes; %u were requested\n", numRead, len);
            ret = E_NOT_SUFFICIENT_BUFFER;
        }
    }

    return ret;
}

HRESULT ReadTargetPointer(PCMD_CTX ctx, WDBG_PTR src, WDBG_PTR *pPtr)
{
    HRESULT ret;
    
    if (ctx->pointerLen == 8)
    {
        ULONG64 ptr64;
        ret = ReadVirtual(ctx, src, &ptr64, sizeof(ptr64));
        *pPtr = ptr64;
    }
    else
    {
        LONG32 ptr32;
        ret = ReadVirtual(ctx, src, &ptr32, sizeof(ptr32));
        *pPtr = SIGN_EXTEND32(ptr32);
    }

    return ret;
}

HRESULT ReadTargetPointerArray(PCMD_CTX ctx, WDBG_PTR src, WDBG_PTR *pPtrList, DWORD num)
{
    HRESULT ret;
    DWORD i;

    for (i = 0; i < num; i++)
    {
        if (FAILED(ret = ReadTargetPointer(ctx, src + (i * ctx->pointerLen), &pPtrList[i])))
            break;
    }

    return ret;
}



char *FlattenArgv(char **argv, int argc)
{
    char *flat, *p;
    int i;
    size_t len = 0;

    // calculate the size of the resulting string
    for (i = 0; i < argc; i++)
        len += strlen(argv[i]) + 1;
    
    // allocate a string
    if ((flat = (char *) malloc(len + 1)) == NULL)
        return NULL;

    // copy the argv members into flat
    for (p = flat, i = 0; i < argc; i++)
    {
        strcpy(p, argv[i]); 
        p += strlen(argv[i]);
        strcpy(p, " ");
        p++;
    }
    *p = '\0';

    return flat;
}

//
// get an expression from an array of char *
//
WDBG_PTR GetExpressionArgv(char **argv, int argc)
{
    WDBG_PTR ret = 0;

    // flatten the argv into a single string
    char *flat = FlattenArgv(argv, argc);
    if (flat != NULL)
    {
        // get the expression
        ret = GetExpression(flat);
        free(flat);
    }

    return ret;
}