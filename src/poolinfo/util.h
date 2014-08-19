#pragma once

// utility functions
extern PCHAR* CommandLineToArgvA(PCHAR CmdLine,int* _argc);
extern char **MakeCmdline(PCSTR argv0, PCSTR args, int* pargc);
extern HRESULT OutputDmlBase(PDEBUG_CONTROL4 pDebugControl, const char *fmt, va_list ap);
extern HRESULT OutputDml(PCMD_CTX ctx, _In_ PCSTR Format, ...);
extern HRESULT ReadVirtual(PCMD_CTX ctx, WDBG_PTR src, PVOID dst, ULONG len);
extern HRESULT ReadTargetPointer(PCMD_CTX ctx, WDBG_PTR src, WDBG_PTR *pPtr);
extern HRESULT ReadTargetPointerArray(PCMD_CTX ctx, WDBG_PTR src, WDBG_PTR *pPtrList, DWORD num);
extern char *FlattenArgv(char **argv, int argc);
extern WDBG_PTR GetExpressionArgv(char **argv, int argc);
