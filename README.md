poolinfo
========

A handy windbg extenstion for Windows kernel pool introspection

Usage
-----

```
                    _ _        __       
  _ __   ___   ___ | (_)_ __  / _| ___  
 | '_ \ / _ \ / _ \| | | '_ \| |_ / _ \ 
 | |_) | (_) | (_) | | | | | |  _| (_) |
 | .__/ \___/ \___/|_|_|_| |_|_|  \___/ 
 |_|                                    
        by: jfisher @debugregister

!poollist [options] [pool types]
  types:
    -a, --all              display all pool descriptors (default)
    -n, --nonpaged         display the NonPaged pool descriptors
    -p, --paged            display the Paged pool descriptors
    -s, --session          display the Session pool descriptors
  options:
    -l, --lookaside        display the lookaside descriptors as well

!poolinfo [options] <descriptor|lookaside address>
  pool options:
    -s, --summary          show a summary of the pool descriptor (default)
    -v, --verbose          display a full dump of the pool descriptor
    -f, --free             show the freelist (ListHeads)
    -b, --bucket <size>    the bucket to show in the freelist (default is all)
  lookaside options:
    -l, --lookaside        treat the supplied address as a lookaside pointer
    -t, --looktype <type>  the lookaside type. one of: nonpaged, paged, session
                           this option specifies the # of buckets in the list
                           and the size of the lookaside structure itself
    -b, --bucket <size>    the bucket to show from the lookaside
  other lookaside options
    -lb, --numbuckets <#>  override the number of lookaside buckets
    -ls, --looksize <size> override the size of the lookaside structure

!poolpage [options] address
  options:
    -r, --raw              display the raw POOL_HEADER structures

!poolchunk <address>
```

Example Usage
=============

List pool descriptors
---------------------

```
kd> !poollist
Pool Descriptors
 NonPagedPoolNx[0]:             fffff8002e100d80
 NonPagedPool[0]:               fffff8002e101ec0
 PagedPool[0]:                  ffffe000bfc3a000
 PagedPool[1]:                  ffffe000bfc3b140
 PagedPool[2]:                  ffffe000bfc3c280
 PagedPool[3]:                  ffffe000bfc3d3c0
 PagedPool[4]:                  ffffe000bfc3e500
 PagedPoolSession[0]:           ffffd0011d4b0d00
```

```
kd> !poollist -l
Pool Descriptors
 NonPagedPoolNx[0]:             fffff8002e100d80
 NonPagedPool[0]:               fffff8002e101ec0
 PagedPool[0]:                  ffffe000bfc3a000
 PagedPool[1]:                  ffffe000bfc3b140
 PagedPool[2]:                  ffffe000bfc3c280
 PagedPool[3]:                  ffffe000bfc3d3c0
 PagedPool[4]:                  ffffe000bfc3e500
 PagedPoolSession[0]:           ffffd0011d4b0d00
Lookaside Descriptors
 Lookaside NonPagedPoolNx[0]:   fffff8002e117a00
 Lookaside NonPagedPool[0]:     fffff8002e118600
 Lookaside PagedPool[0]:        fffff8002e119200
 Lookaside PagedPoolSession[0]: ffffd0011d4b00c0
```


Get pool descriptor and lookaside information
---------------------------------------------

### Display the descriptor itself

```
kd> !poolinfo -v fffff8002e100d80
Pool Descriptor NonPagedPoolNx[0] at fffff8002e100d80
 PoolType:                     200 (NonPagedPoolNx)
 PoolIndex:                    00000000
 PendingFreeDepth:             0000001B
 RunningAllocs:                00068798
 RunningDeAllocs:              000562CE
 TotalBigPages:                00001097
 ThreadsProcessingDeferrals:   00000000
 TotalBytes:                   02410E00
 TotalPages:                   0000146A
```

### Display the pool freelist (ListHeads)

```
kd> !poolinfo -v -f fffff8002e100d80
Pool Descriptor NonPagedPoolNx[0] at fffff8002e100d80
 PoolType:                     200 (NonPagedPoolNx)
 PoolIndex:                    00000000
 PendingFreeDepth:             0000001B
 RunningAllocs:                00068798
 RunningDeAllocs:              000562CE
 TotalBigPages:                00001097
 ThreadsProcessingDeferrals:   00000000
 TotalBytes:                   02410E00
 TotalPages:                   0000146A
ListHeads[01]: size=020
  ffffe000c21a1f90: size:020 prev:0E0 index:00 type:00 tag:Free
  ffffe000c1ad8940: size:020 prev:260 index:00 type:00 tag:Free
  ffffe000c1ad8e20: size:020 prev:090 index:00 type:00 tag:Free
  ffffe000c19e50e0: size:020 prev:0E0 index:00 type:00 tag:Free
  ffffe000c1980e50: size:020 prev:250 index:00 type:00 tag:Free
  ffffe000c1ade150: size:020 prev:150 index:00 type:00 tag:Free
  ffffe000bfe2dee0: size:020 prev:080 index:00 type:00 tag:Free
  ffffe000c219f3b0: size:020 prev:220 index:00 type:00 tag:Free
  ffffe000c1ae38b0: size:020 prev:090 index:00 type:00 tag:Free
  ffffe000c1ae8c10: size:020 prev:090 index:00 type:00 tag:Free
  ffffe000c1ad9d30: size:020 prev:0D0 index:00 type:00 tag:Free
  ffffe000c1ae65e0: size:020 prev:090 index:00 type:00 tag:Free
  ffffe000c1ae8170: size:020 prev:090 index:00 type:00 tag:Free
  ffffe000c1aeeeb0: size:020 prev:090 index:00 type:00 tag:Free
  ffffe000c1aff810: size:020 prev:810 index:00 type:00 tag:Free
  ffffe000c1b01eb0: size:020 prev:0E0 index:00 type:00 tag:Free
  ffffe000c1b077f0: size:020 prev:170 index:00 type:00 tag:Free
  ffffe000c1aeeb30: size:020 prev:230 index:00 type:00 tag:Free
  ffffe000c19e5370: size:020 prev:0A0 index:00 type:00 tag:Free
  ffffe000c1af9560: size:020 prev:090 index:00 type:00 tag:Free
  ffffe000c1aef220: size:020 prev:220 index:00 type:00 tag:Free
  ffffe000c1aef4d0: size:020 prev:150 index:00 type:00 tag:Free
  ffffe000c1afd080: size:020 prev:080 index:00 type:00 tag:Free
  ffffe000c1b0aa10: size:020 prev:230 index:00 type:00 tag:Free
  ffffe000c1b15190: size:020 prev:190 index:00 type:00 tag:Free
  ffffe000c2082510: size:020 prev:110 index:00 type:00 tag:Free
  ffffe000c1b0f400: size:020 prev:400 index:00 type:00 tag:Free
  ffffe000c1b0ae00: size:020 prev:0E0 index:00 type:00 tag:Free
  ffffe000c1b10400: size:020 prev:400 index:00 type:00 tag:Free
  ffffe000c1b14be0: size:020 prev:090 index:00 type:00 tag:Free
  ffffe000c1b1a130: size:020 prev:130 index:00 type:00 tag:Free
  ...SNIP...
```

### Display only free list for size 0xBD0

```
kd> !poolinfo -f -b BD0 fffff8002e100d80
Pool Descriptor NonPagedPoolNx[0] at fffff8002e100d80
 PoolType:                     200 (NonPagedPoolNx)
 PoolIndex:                    00000000
 PendingFreeDepth:             0000001B
ListHeads[BC]: size=BD0
  ffffe000c025d430: size:BD0 prev:060 index:00 type:00 tag:FMic
  ffffe000c0245430: size:BD0 prev:060 index:00 type:00 tag:FMic
  ffffe000c025f430: size:BD0 prev:060 index:00 type:00 tag:FMic
  ffffe000c02e3430: size:BD0 prev:060 index:00 type:00 tag:FMic
  ffffe000c0243430: size:BD0 prev:060 index:00 type:00 tag:FMic
  ffffe000c023d430: size:BD0 prev:060 index:00 type:00 tag:Free
```

### Display some lookaside bucket

```
kd> !poolinfo -l -t Paged -b 180 fffff8002e119200
Lookaside[17]: size=180, fffff8002e119aa0
  ffffc0000b94fa50: size:180 prev:010 index:01 type:05 tag:PfRL
  ffffc000093a3e40: size:180 prev:010 index:04 type:05 tag:FMfn
```

### Display contents of a pool page

This is much like the build-in *!pool* command but a bit easier to read at a glance.

```
kd> !poolpage ffffc0000b94fa50
walking pool page @ ffffc0000b94f000
 Addr              A/F   BlockSize     PreviousSize  PoolIndex PoolType Tag
 ---------------------------------------------------------------------------
 ffffc0000b94f000: InUse 0260 (026)    0000 (000)           01       03 FMfn
 ffffc0000b94f260: InUse 0170 (017)    0260 (026)           01       03 NtFs
 ffffc0000b94f3d0: InUse 0280 (028)    0170 (017)           01       03 FMfn
 ffffc0000b94f650: InUse 0060 (006)    0280 (028)           01       03 CMNb
 ffffc0000b94f6b0: InUse 00C0 (00C)    0060 (006)           01       03 FIcs
 ffffc0000b94f770: InUse 0050 (005)    00C0 (00C)           01       03 CMNb
 ffffc0000b94f7c0: InUse 0070 (007)    0050 (005)           01       03 CMNb
 ffffc0000b94f830: InUse 0050 (005)    0070 (007)           01       03 CMNb
 ffffc0000b94f880: InUse 0050 (005)    0050 (005)           01       03 CMNb
 ffffc0000b94f8d0: InUse 0040 (004)    0050 (005)           01       03 CMNb
 ffffc0000b94f910: InUse 0060 (006)    0040 (004)           01       03 CMNb
 ffffc0000b94f970: InUse 0060 (006)    0060 (006)           01       03 CMNb
 ffffc0000b94f9d0: InUse 0070 (007)    0060 (006)           01       0B DxgK
 ffffc0000b94fa40: Free  0010 (001)    0070 (007)           01       00 Free
*ffffc0000b94fa50: Free  0180 (018)    0010 (001)           01       05 PfRL
 ffffc0000b94fbd0: InUse 01D0 (01D)    0180 (018)           01       03 FMfn
 ffffc0000b94fda0: InUse 0260 (026)    01D0 (01D)           01       03 FMfn
```


