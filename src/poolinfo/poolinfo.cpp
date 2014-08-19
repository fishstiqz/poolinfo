#include "stdafx.h"

#include "SimpleOpt.h"

//#define USE_TABULAR_FOR_FREELIST

// globals
WINDBG_EXTENSION_APIS   ExtensionApis = {0};

#ifdef _DEBUG
BOOL g_bDebug = TRUE;
#else
BOOL g_bDebug = FALSE;
#endif



// command line option ids
CSimpleOptA::SOption g_pp_opts[] = {
    HELP_OPTS,
    { OPT_PP_RAWHDR,    "-r",       SO_NONE },
    { OPT_PP_RAWHDR,    "--raw",    SO_NONE },
    SO_END_OF_OPTIONS
};

void Usage_CmdPoolPage(PCMD_CTX ctx)
{
    OutputDml(ctx, "<b>!poolpage</b>"); dprintf(" [options] address\n");
    dprintf(
        "  options:\n"
        "    -r, --raw              display the raw POOL_HEADER structures\n"
        );
}

HRESULT CALLBACK poolpage(PDEBUG_CLIENT4 pClient, PCSTR args)
{
    HRESULT ret = S_OK;

    CMD_CTX ctx;
    CSimpleOptA cs;

    char **argv = NULL;
    int argc;
    BOOL bRaw = FALSE;

    WDBG_PTR address = 0;
    WDBG_PTR page;

    BYTE pageContents[PAGE_SIZE];
 

    // initialize the command context
    if (FAILED(ret = InitCmdCtx(&ctx, pClient)))
        return ret;

    // parse !poolpage options
    argv = MakeCmdline("!poolpage", args, &argc);
    if (argv == NULL)
    {
        ret = E_OUTOFMEMORY;
        goto cleanup;
    }   
    
    cs.Init(argc, argv, g_pp_opts);
    while (cs.Next())
    {
        if (cs.LastError() == SO_SUCCESS)
        {
            // handle options
            switch (cs.OptionId())
            {
            default:
            case OPT_HELP:
                Usage_CmdPoolPage(&ctx);
                goto cleanup;
                break;
            case OPT_PP_RAWHDR:
                bRaw = TRUE;
                break;
            }
        }
    }
    
    // make sure an address is specified
    if (cs.FileCount() == 0)
    {
        Usage_CmdPoolPage(&ctx);
        goto cleanup;
    }

    // resolve the expression
    address = GetExpressionArgv(cs.Files(), cs.FileCount());

    // get the page base
    page = PAGE_ALIGN(address);

    // fetch the entire pool page
    if (FAILED(ret = ReadVirtual(&ctx, page, pageContents, PAGE_SIZE)))
    {
        dprintf("invalid address: %p\n", page);
        ret = S_OK;
        goto cleanup;
    }

    // start walking the page
    dprintf("walking pool page @ %p\n", page);

    // enum the pool page
    PRINT_OPTS printOpts;
    memset(&printOpts, 0, sizeof(printOpts));
    printOpts.bHighlight    = TRUE;
    printOpts.bRaw          = bRaw;
    printOpts.bTabular      = TRUE;
    printOpts.indentLevel   = 0;
    printOpts.kPtr          = address;
    printOpts.count         = 0;

    EnumPoolPage(&ctx, address, (LPARAM)&printOpts, EnumProcPrintHeader);
    
cleanup:
    if (argv != NULL)
        free(argv);

    ReleaseCmdCtx(&ctx);
    return ret;
}


void PrintPoolDescInfo(PCMD_CTX ctx, PPOOL_DESC pPoolDesc)
{
    char buf[50];
    const char *name;
    ULONG poolIndex;

    name = GetPoolType(ctx, pPoolDesc->kPoolDesc, &poolIndex);
    sprintf_s(buf, sizeof(buf), "%s[%u]:", (name ? name : "Unknown"), poolIndex);

    OutputDml(ctx, 
        " %-30s <link cmd=\"!poolinfo -v -f %p\">%p</link>\n",
       buf, pPoolDesc->kPoolDesc, pPoolDesc->kPoolDesc);
}

void PrintLookasideInfo(PCMD_CTX ctx, PLOOKASIDE_DESC pLookasideDesc)
{
    char buf[50];
    const char *name;
    const char *infotype = GetLookasideInfoType(pLookasideDesc->poolType);

    name = GetLookasideType(ctx, pLookasideDesc->kLookasideDesc);
    sprintf_s(buf, sizeof(buf), "Lookaside %s[%u]:", (name ? name : "Unknown"), pLookasideDesc->processor);

    OutputDml(ctx, 
        " %-30s <link cmd=\"!poolinfo -s -l -t %s %p\">%p</link>\n",
        buf,
        infotype,
        pLookasideDesc->kLookasideDesc, pLookasideDesc->kLookasideDesc);
}

CSimpleOptA::SOption g_pl_opts[] = {
    HELP_OPTS,
    { OPT_PL_ALL,       "-a",           SO_NONE },
    { OPT_PL_ALL,       "--all",        SO_NONE },
    { OPT_PL_NONPAGED,  "-n",           SO_NONE },
    { OPT_PL_NONPAGED,  "--nonpaged",   SO_NONE },
    { OPT_PL_PAGED,     "-p",           SO_NONE },
    { OPT_PL_PAGED,     "--paged",      SO_NONE },
    { OPT_PL_SESSION,   "-s",           SO_NONE },
    { OPT_PL_SESSION,   "--session",    SO_NONE },
    { OPT_PL_LOOKASIDE, "-l",           SO_NONE },
    { OPT_PL_LOOKASIDE, "--lookaside",  SO_NONE },
    SO_END_OF_OPTIONS
};


void Usage_CmdPoolList(PCMD_CTX ctx)
{
    OutputDml(ctx, "<b>!poollist</b>"); dprintf(" [options] [pool types]\n");
    dprintf(
        "  types:\n"
        "    -a, --all              display all pool descriptors (default)\n"
        "    -n, --nonpaged         display the NonPaged pool descriptors\n"
        "    -p, --paged            display the Paged pool descriptors\n"
        "    -s, --session          display the Session pool descriptors\n"
        "  options:\n"
        "    -l, --lookaside        display the lookaside descriptors as well\n"
        );
}

HRESULT CALLBACK poollist(PDEBUG_CLIENT4 pClient, PCSTR args)
{
    HRESULT ret = S_OK;
       
    CMD_CTX ctx;
    CSimpleOptA cs;

    char **argv = NULL;
    int argc;
    
    DWORD poolTypes = 0;
    BOOL bLookaside = FALSE;

    DWORD i;
    DWORD numDescs;
    
    POOL_DESC *pPoolDescs = NULL;
    LOOKASIDE_DESC *pLookasideDescs = NULL;

    // initialize the command context
    if (FAILED(ret = InitCmdCtx(&ctx, pClient)))
        return ret;

    // parse !poolpage options
    argv = MakeCmdline("!poollist", args, &argc);
    if (argv == NULL)
    {
        ret = E_OUTOFMEMORY;
        goto cleanup;
    }

    cs.Init(argc, argv, g_pl_opts);
    while (cs.Next())
    {
        if (cs.LastError() == SO_SUCCESS)
        {
            // handle options
            switch (cs.OptionId())
            {
            default:
            case OPT_HELP:
                Usage_CmdPoolList(&ctx);
                goto cleanup;
                break;
            case OPT_PL_NONPAGED:
                poolTypes |= POOL_TYPE_NONPAGED;
                break;
            case OPT_PL_PAGED:
                poolTypes |= POOL_TYPE_PAGED;
                break;
            case OPT_PL_SESSION:
                poolTypes |= POOL_TYPE_SESSION;
                break;
            case OPT_PL_ALL:
                poolTypes = 0xffffffff;
                break;
            case OPT_PL_LOOKASIDE:
                bLookaside = TRUE;
                break;
            }
        }
    }

    // if none specified, do all
    if (poolTypes == 0)
        poolTypes = 0xffffffff;

    // get all the pool descriptors
    pPoolDescs = GetAllPoolDescriptors(&ctx, &numDescs);
    if (pPoolDescs != NULL)
    {
        dprintf("Pool Descriptors\n");
        for (i = 0; i < numDescs; i++)
        {
            if (pPoolDescs[i].poolType & poolTypes)
                PrintPoolDescInfo(&ctx, &pPoolDescs[i]);
        }
    }

    if (bLookaside)
    {
        pLookasideDescs = GetAllLookasides(&ctx, &numDescs);
        if (pLookasideDescs != NULL)
        {
            dprintf("Lookaside Descriptors\n");
            for (i = 0; i < numDescs; i++)
            {
                if (pLookasideDescs[i].poolType & poolTypes)
                    PrintLookasideInfo(&ctx, &pLookasideDescs[i]);
            }
        }
    }

cleanup:
    if (pPoolDescs != NULL)
        free(pPoolDescs);

    if (pLookasideDescs != NULL)
        free(pLookasideDescs);

    if (argv != NULL)
        free(argv);

    ReleaseCmdCtx(&ctx);
    return ret;
}

BOOL CALLBACK EnumListHeadsProcPrint(PCMD_CTX ctx, ULONG bucket, BOOL bEmpty, WDBG_PTR kChunk, ULONG count, LPARAM lParam)
{
    PPRINT_OPTS print = (PPRINT_OPTS)lParam;

    if (bEmpty)
    {
        if (print->bPrintIfEmpty)
        {
            dprintf("ListHeads[%02X]: size=%03X\n", bucket, BUCKET_TO_SIZE(bucket, ctx->poolBlockSize));
            for (ULONG i = 0; i < print->indentLevel; i++) dprintf(" ");
            dprintf("Empty\n");
        }
    }
    else
    {
        if (count == 0)
            dprintf("ListHeads[%02X]: size=%03X\n", bucket, BUCKET_TO_SIZE(bucket, ctx->poolBlockSize));

        if (print->bTabular)
            PrintTabularChunkHeader(ctx, print->indentLevel);

        PrintPoolHeader(ctx, kChunk, NULL, print->bTabular, print->bRaw, print->indentLevel, print->bHighlight);
    }

    return TRUE;
}


HRESULT PrintPoolDescriptor(PCMD_CTX ctx, WDBG_PTR ptr, BOOL bSummary)
{
    HRESULT ret;
    PSYM_FIELD pStructDesc;
    PVOID pStructData = NULL;

    pStructDesc = CloneStruct(ctx->pStructPoolDescriptor);
    if (pStructDesc == NULL)
        return E_OUTOFMEMORY;

    if (FAILED(ret = ReadVirtualStruct(ctx, ptr, pStructDesc, &pStructData)))
    {
        free(pStructDesc);
        return ret;
    }

    // print the descriptor details
    dprintf("Pool Descriptor %s[%u] at %p\n",
        PoolTypeName((POOL_TYPE) GS_ULONG(pStructDesc, "PoolType")),
        GS_ULONG32(pStructDesc, "PoolIndex"),
        ptr);

    dprintf(" PoolType:                     %02X (%s)\n",
        GS_ULONG(pStructDesc, "PoolType"),
        PoolTypeName((POOL_TYPE) GS_ULONG(pStructDesc, "PoolType")));

    dprintf(" PoolIndex:                    %08X\n", GS_ULONG32(pStructDesc, "PoolIndex"));
    dprintf(" PendingFreeDepth:             %08X\n", GS_LONG32(pStructDesc, "PendingFreeDepth"));

    if (!bSummary)
    {
        dprintf(" RunningAllocs:                %08X\n", GS_LONG32(pStructDesc, "RunningAllocs"));
        dprintf(" RunningDeAllocs:              %08X\n", GS_LONG32(pStructDesc, "RunningDeAllocs"));
        dprintf(" TotalBigPages:                %08X\n", GS_LONG32(pStructDesc, "TotalBigPages"));
        dprintf(" ThreadsProcessingDeferrals:   %08X\n", GS_LONG32(pStructDesc, "ThreadsProcessingDeferrals"));
        dprintf(" TotalBytes:                   %08X\n", GS_ULONG32(pStructDesc, "TotalBytes"));
        dprintf(" TotalPages:                   %08X\n", GS_ULONG32(pStructDesc, "TotalPages"));
    }

    free(pStructDesc);
    free(pStructData);

    return ret;
}

HRESULT PrintLookaside(PCMD_CTX ctx, WDBG_PTR ptr, BOOL bSummary)
{
    HRESULT ret;
    PSYM_FIELD pStructDesc = NULL;
    PVOID pStructData = NULL;
    char tag[5];

    pStructDesc = CloneStruct(ctx->pStructGeneralLookasidePool);
    if (pStructDesc == NULL)
        return E_OUTOFMEMORY;

    if (FAILED(ret = ReadVirtualStruct(ctx, ptr, pStructDesc, &pStructData)))
    {
        free(pStructDesc);
        return ret;
    }

    // print the lookaside details
    dprintf(" Type:                         %02X (%s)\n",
        GS_ULONG(pStructDesc, "Type"),
        PoolTypeName((POOL_TYPE) GS_ULONG(pStructDesc, "Type")));

    GetPoolTag(GS_ULONG32(pStructDesc, "Tag"), tag);
    dprintf(" Size:                         %08X\n", GS_ULONG32(pStructDesc, "Size"));
    dprintf(" Tag:                          %08X (%s)\n", GS_ULONG32(pStructDesc, "Tag"), tag);

    if (!bSummary)
    {
        dprintf(" Depth:                        %04X\n", GS_WORD(pStructDesc, "Depth"));
        dprintf(" MaximumDepth:                 %04X\n", GS_WORD(pStructDesc, "MaximumDepth"));
        //dprintf(" AllocateMisses:               %08X\n", GS_LONG32(pStructDesc, "AllocateMisses"));
        dprintf(" AllocateHits:                 %08X\n", GS_ULONG32(pStructDesc, "AllocateHits"));
        dprintf(" TotalFrees:                   %08X\n", GS_ULONG32(pStructDesc, "TotalFrees"));
        //dprintf(" FreeMisses:                   %08X\n", GS_ULONG32(pStructDesc, "FreeMisses"));
        dprintf(" FreeHits:                     %08X\n", GS_ULONG32(pStructDesc, "FreeHits"));
    }

    free(pStructDesc);
    free(pStructData);

    return ret;
}

BOOL FindLookasideFromPointer(PCMD_CTX ctx, WDBG_PTR kLookaside, PLOOKASIDE_DESC pLookasideDesc)
{
    BOOL bRet = FALSE;
    ULONG i, num = 0;
    PLOOKASIDE_DESC pDescs;

    pDescs = GetAllLookasides(ctx, &num);
    if (pDescs != NULL)
    {
        for (i = 0; i < num; i++)
        {
            if (pDescs[i].kLookasideDesc == kLookaside)
            {
                memcpy(pLookasideDesc, &pDescs[i], sizeof(LOOKASIDE_DESC));
                bRet = TRUE;
                break;
            }
        }

        free(pDescs);
    }

    return bRet;
}

BOOL CALLBACK EnumLookasideProcPrint(PCMD_CTX ctx, WDBG_PTR kLookasideBucket, ULONG bucket, BOOL bEmpty, WDBG_PTR kChunk, ULONG count, LPARAM lParam)
{
    PPRINT_OPTS print = (PPRINT_OPTS)lParam;

    if (count == 0 && (!bEmpty || (bEmpty && print->bPrintIfEmpty)))
    {
        dprintf("Lookaside[%02X]: size=%03X, %p\n", bucket, BUCKET_TO_SIZE(bucket, ctx->poolBlockSize), kLookasideBucket);
        // XXX: print more header stuff here...
        if (!print->bSummary)
            PrintLookaside(ctx, kLookasideBucket, FALSE);
    }

    if (bEmpty)
    {
        if (print->bPrintIfEmpty)
        {
            for (ULONG i = 0; i < print->indentLevel; i++) dprintf(" ");
            dprintf("Empty\n");
        }
    }
    else
    {
        if (print->bTabular)
            PrintTabularChunkHeader(ctx, print->indentLevel);

        PrintPoolHeader(ctx, kChunk, NULL, print->bTabular, print->bRaw, print->indentLevel, print->bHighlight);
    }

    return TRUE;
}

CSimpleOptA::SOption g_pi_opts[] = {
    HELP_OPTS,
    { OPT_PI_SUMMARY,   "-s",           SO_NONE },
    { OPT_PI_SUMMARY,   "--summary",    SO_NONE },
    { OPT_PI_VERBOSE,   "-v",           SO_NONE },
    { OPT_PI_VERBOSE,   "--verbose",    SO_NONE },
    { OPT_PI_FREELIST,  "-f",           SO_NONE },
    { OPT_PI_FREELIST,  "--free",       SO_NONE },
    { OPT_PI_BUCKET,    "-b",           SO_REQ_SEP },
    { OPT_PI_BUCKET,    "--bucket",     SO_REQ_SEP },
    { OPT_PI_LOOKASIDE, "-l",           SO_NONE },
    { OPT_PI_LOOKASIDE, "--lookaside",  SO_NONE },
    { OPT_PI_LOOKTYPE,  "-t",           SO_REQ_SEP },
    { OPT_PI_LOOKTYPE,  "--looktype",   SO_REQ_SEP },
    { OPT_PI_NUMBUCKETS,"-lb",          SO_REQ_SEP },
    { OPT_PI_NUMBUCKETS,"--numbuckets", SO_REQ_SEP },
    { OPT_PI_LOOKSIZE,  "-ls",          SO_REQ_SEP },
    { OPT_PI_LOOKSIZE,  "--looksize",   SO_REQ_SEP },
    SO_END_OF_OPTIONS
};


void Usage_CmdPoolInfo(PCMD_CTX ctx)
{
    OutputDml(ctx, "<b>!poolinfo</b>"); dprintf(" [options] <descriptor|lookaside address>\n");
    dprintf(
        "  pool options:\n"
        "    -s, --summary          show a summary of the pool descriptor (default)\n"
        "    -v, --verbose          display a full dump of the pool descriptor\n"
        "    -f, --free             show the freelist (ListHeads)\n"
        "    -b, --bucket <size>    the bucket to show in the freelist (default is all)\n"
        "  lookaside options:\n"
        "    -l, --lookaside        treat the supplied address as a lookaside pointer\n"
        "    -t, --looktype <type>  the lookaside type. one of: nonpaged, paged, session\n"
        "                           this option specifies the # of buckets in the list\n"
        "                           and the size of the lookaside structure itself\n"
        "    -b, --bucket <size>    the bucket to show from the lookaside\n"
        "  other lookaside options\n"
        "    -lb, --numbuckets <#>  override the number of lookaside buckets\n"
        "    -ls, --looksize <size> override the size of the lookaside structure\n"
        );
}


HRESULT CALLBACK poolinfo(PDEBUG_CLIENT4 pClient, PCSTR args)
{
    HRESULT ret = S_OK;   
    CMD_CTX ctx;
    CSimpleOptA cs;

    char **argv = NULL;
    int argc;

    // options
    BOOL bSummary = TRUE;
    BOOL bFreeList = FALSE;
    BOOL bLookaside = FALSE;
    ULONG sizeBucket = BUCKET_ALL;
    WDBG_PTR address = 0;
    ULONG lookasideStructSize = 0;
    ULONG lookasideBuckets = 0;
    ULONG bucket;

    // initialize the command context
    if (FAILED(ret = InitCmdCtx(&ctx, pClient)))
        return ret;

    // parse !poolpage options
    argv = MakeCmdline("!poolinfo", args, &argc);
    if (argv == NULL)
    {
        ret = E_OUTOFMEMORY;
        goto cleanup;
    }

    cs.Init(argc, argv, g_pi_opts);
    while (cs.Next())
    {
        if (cs.LastError() == SO_SUCCESS)
        {
            // handle options
            switch (cs.OptionId())
            {
            default:
            case OPT_HELP:
                Usage_CmdPoolInfo(&ctx);
                goto cleanup;
            case OPT_PI_SUMMARY:
                bSummary = TRUE;
                break;
            case OPT_PI_VERBOSE:
                bSummary = FALSE;
                break;
            case OPT_PI_FREELIST:
                bFreeList = TRUE;
                break;
            case OPT_PI_BUCKET:
                sizeBucket = strtoul(cs.OptionArg(), NULL, 16);
                break;
            case OPT_PI_LOOKASIDE:
                bLookaside = TRUE;
                break;
            case OPT_PI_LOOKTYPE:
                if ((_stricmp(cs.OptionArg(), "nonpaged") == 0) ||
                    (_stricmp(cs.OptionArg(), "paged") == 0))
                {
                    lookasideStructSize = ctx.lookasideStructSize;
                    lookasideBuckets = ctx.lookasideBuckets;
                }
                else if (_stricmp(cs.OptionArg(), "session") == 0)
                {
                    lookasideStructSize = ctx.sessionLookasideStructSize;
                    lookasideBuckets = ctx.sessionLookasideBuckets;
                }
                else
                {
                    // bad argument
                    Usage_CmdPoolInfo(&ctx);
                    goto cleanup;
                }
                break;
            case OPT_PI_NUMBUCKETS:
                lookasideBuckets = strtoul(cs.OptionArg(), NULL, 16);
            case OPT_PI_LOOKSIZE:
                lookasideStructSize = strtoul(cs.OptionArg(), NULL, 16);
                break;
            }
        }
    }

    // do the arguments make sense?
    if (bLookaside)
    {
        // XXX: convert sizeBucket from size to bucket
        bucket = SIZE_TO_BUCKET(sizeBucket, ctx.poolBlockSize);

        if (lookasideBuckets == 0 || lookasideStructSize == 0)
        {
            Usage_CmdPoolInfo(&ctx);
            goto cleanup;
        }

        if (sizeBucket != BUCKET_ALL && bucket >= lookasideBuckets)
        {
            dprintf("bucket size is too large. must be between 0 and %02X\n\n",
                BUCKET_TO_SIZE(lookasideBuckets-1, ctx.poolBlockSize));
            Usage_CmdPoolInfo(&ctx);
            goto cleanup;
        }
    }

    // make sure an address is specified
    if (cs.FileCount() == 0)
    {
        Usage_CmdPoolInfo(&ctx);
        goto cleanup;
    }

    // resolve the expression
    address = GetExpressionArgv(cs.Files(), cs.FileCount());

    if (bLookaside)
    {
        LOOKASIDE_DESC lookasideDesc = {0};
        if (!FindLookasideFromPointer(&ctx, address, &lookasideDesc))
        {
            dprintf("%p doesn't look like a real lookaside address, aborting\n", address);
        }
        else
        {
            bucket = sizeBucket;
            if (bucket != BUCKET_ALL)
                bucket = SIZE_TO_BUCKET(bucket, ctx.poolBlockSize);

            PRINT_OPTS print = {0};
            print.indentLevel = 2;
            print.bPrintIfEmpty = FALSE;
            print.bSummary = bSummary;

            EnumLookasides(&ctx, &lookasideDesc, bucket, (LPARAM)&print, EnumLookasideProcPrint);
        }
    }
    else
    {
        // the pointer was a pool descriptor

        // dump the pool descriptor
        PrintPoolDescriptor(&ctx, address, bSummary);

        // if specified, dump the freelist(s)
        if (bFreeList)
        {
            bucket = sizeBucket;
            if (bucket != BUCKET_ALL)
                bucket = SIZE_TO_BUCKET(bucket, ctx.poolBlockSize);

            PRINT_OPTS print = {0};
            print.indentLevel = 2;
            print.bPrintIfEmpty = FALSE;

            EnumListHeads(&ctx, address, bucket, (LPARAM)&print, EnumListHeadsProcPrint);
        }
    }

   
cleanup:
    if (argv != NULL)
        free(argv);

    ReleaseCmdCtx(&ctx);
    return ret;
}



CSimpleOptA::SOption g_pc_opts[] = {
    HELP_OPTS,
    SO_END_OF_OPTIONS
};


void Usage_CmdPoolChunk(PCMD_CTX ctx)
{
    OutputDml(ctx, "<b>!poolchunk</b>"); dprintf(" <address>\n");
}


HRESULT CALLBACK poolchunk(PDEBUG_CLIENT4 pClient, PCSTR args)
{
    HRESULT ret = S_OK;   
    CMD_CTX ctx;
    CSimpleOptA cs;

    char **argv = NULL;
    int argc;

    WDBG_PTR address;

    WDBG_PTR kChunk;

    // initialize the command context
    if (FAILED(ret = InitCmdCtx(&ctx, pClient)))
        return ret;

    // parse !poolpage options
    argv = MakeCmdline("!poolchunk", args, &argc);
    if (argv == NULL)
    {
        ret = E_OUTOFMEMORY;
        goto cleanup;
    }

    cs.Init(argc, argv, g_pi_opts);
    while (cs.Next())
    {
        if (cs.LastError() == SO_SUCCESS)
        {
            // handle options
            switch (cs.OptionId())
            {
            default:
            case OPT_HELP:
                Usage_CmdPoolChunk(&ctx);
                goto cleanup;
            }
        }
    }

    // make sure an address is specified
    if (cs.FileCount() == 0)
    {
        Usage_CmdPoolInfo(&ctx);
        goto cleanup;
    }

    // resolve the expression
    address = GetExpressionArgv(cs.Files(), cs.FileCount());

    // walk through the pool page and find the pool header for
    // this address
    kChunk = FindPoolHeader(&ctx, address);
    if (kChunk != NULL)
    {
        PrintPoolHeader(&ctx, kChunk, NULL, FALSE, FALSE, 0, FALSE);
        PrintExtendedChunkInfo(&ctx, kChunk);
    }

cleanup:
    if (argv != NULL)
        free(argv);

    ReleaseCmdCtx(&ctx);
    return ret;
}

void ShowBanner(PCMD_CTX ctx)
{
    OutputDml(ctx,
        "<b>\n"
        "                    _ _        __       \n"
        "  _ __   ___   ___ | (_)_ __  / _| ___  \n"
        " | '_ \\ / _ \\ / _ \\| | | '_ \\| |_ / _ \\ \n"
        " | |_) | (_) | (_) | | | | | |  _| (_) |\n"
        " | .__/ \\___/ \\___/|_|_|_| |_|_|  \\___/ \n"
        " |_|                                    \n"
        "</b>        by: jfisher @debugregister\n"
        "\n"
        );
        
    Usage_CmdPoolList(ctx);
    dprintf("\n");
    Usage_CmdPoolInfo(ctx);
    dprintf("\n");
    Usage_CmdPoolPage(ctx);
    dprintf("\n");
    Usage_CmdPoolChunk(ctx);
    dprintf("\n");
}

// The entry point for the extension
extern "C" HRESULT CALLBACK DebugExtensionInitialize(PULONG Version, PULONG Flags)
{
    HRESULT ret = S_OK;
    PDEBUG_CLIENT4 pDebugClient;
    PDEBUG_CONTROL pDebugControl;
    CMD_CTX ctx;

    if (FAILED(ret = DebugCreate(__uuidof(IDebugClient4),(PVOID *)&pDebugClient)))
        return ret;

    if (FAILED(ret = pDebugClient->QueryInterface(__uuidof(IDebugControl),(PVOID *)&pDebugControl)))
        return ret;

    ExtensionApis.nSize = sizeof(ExtensionApis);
    //pDebugControl->GetWindbgExtensionApis32((PWINDBG_EXTENSION_APIS32) &ExtensionApis);
    pDebugControl->GetWindbgExtensionApis64((PWINDBG_EXTENSION_APIS64) &ExtensionApis);
    pDebugControl->Release();

    DEBUGPRINT("DebugExtensionInitialize: Retrieved ExtensionApis\n");

    if (SUCCEEDED(ret = InitCmdCtx(&ctx, pDebugClient)))
    {
        ShowBanner(&ctx);
        ReleaseCmdCtx(&ctx);
    }

    pDebugClient->Release();
    return ret;
}


extern "C" HRESULT CALLBACK DebugExtensionUninitialize(void)
{
    DEBUGPRINT("DebugExtensionUninitialize\n");
    return S_OK;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID lpReserved)
{
	switch (reason)
	{
	case DLL_PROCESS_ATTACH:
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
	case DLL_PROCESS_DETACH:
		break;
	}
	return TRUE;
}


