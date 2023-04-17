==2830== Memcheck, a memory error detector
==2830== Copyright (C) 2002-2022, and GNU GPL'd, by Julian Seward et al.
==2830== Using Valgrind-3.20.0 and LibVEX; rerun with -h for copyright info
==2830== Command: bin/deburr
==2830== Parent PID: 1090
==2830== 
==2830== Invalid write of size 4
==2830==    at 0x10DD4A: render (deburr.c:175)
==2830==    by 0x10DE18: frame_done (deburr.c:257)
==2830==    by 0x4A934F5: ffi_call_unix64 (unix64.S:104)
==2830==    by 0x4A8FF5D: ffi_call_int.lto_priv.0 (ffi64.c:673)
==2830==    by 0x4A92B72: ffi_call (ffi64.c:710)
==2830==    by 0x4890644: wl_closure_invoke.constprop.0 (connection.c:1025)
==2830==    by 0x4890E72: dispatch_event.isra.0 (wayland-client.c:1631)
==2830==    by 0x489113B: UnknownInlinedFun (wayland-client.c:1777)
==2830==    by 0x489113B: wl_display_dispatch_queue_pending (wayland-client.c:2019)
==2830==    by 0x10E1F3: main (deburr.c:291)
==2830==  Address 0x5402000 is not stack'd, malloc'd or (recently) free'd
==2830== 
==2830== 
==2830== Process terminating with default action of signal 11 (SIGSEGV): dumping core
==2830==  Access not within mapped region at address 0x5402000
==2830==    at 0x10DD4A: render (deburr.c:175)
==2830==    by 0x10DE18: frame_done (deburr.c:257)
==2830==    by 0x4A934F5: ffi_call_unix64 (unix64.S:104)
==2830==    by 0x4A8FF5D: ffi_call_int.lto_priv.0 (ffi64.c:673)
==2830==    by 0x4A92B72: ffi_call (ffi64.c:710)
==2830==    by 0x4890644: wl_closure_invoke.constprop.0 (connection.c:1025)
==2830==    by 0x4890E72: dispatch_event.isra.0 (wayland-client.c:1631)
==2830==    by 0x489113B: UnknownInlinedFun (wayland-client.c:1777)
==2830==    by 0x489113B: wl_display_dispatch_queue_pending (wayland-client.c:2019)
==2830==    by 0x10E1F3: main (deburr.c:291)
==2830==  If you believe this happened as a result of a stack
==2830==  overflow in your program's main thread (unlikely but
==2830==  possible), you can try to increase the size of the
==2830==  main thread stack using the --main-stacksize= flag.
==2830==  The main thread stack size used in this run was 8388608.
==2830== 
==2830== HEAP SUMMARY:
==2830==     in use at exit: 29,432 bytes in 77 blocks
==2830==   total heap usage: 229 allocs, 152 frees, 58,805 bytes allocated
==2830== 
==2830== LEAK SUMMARY:
==2830==    definitely lost: 5,184 bytes in 27 blocks
==2830==    indirectly lost: 0 bytes in 0 blocks
==2830==      possibly lost: 220 bytes in 1 blocks
==2830==    still reachable: 24,028 bytes in 49 blocks
==2830==         suppressed: 0 bytes in 0 blocks
==2830== Rerun with --leak-check=full to see details of leaked memory
==2830== 
==2830== For lists of detected and suppressed errors, rerun with: -s
==2830== ERROR SUMMARY: 1 errors from 1 contexts (suppressed: 0 from 0)
