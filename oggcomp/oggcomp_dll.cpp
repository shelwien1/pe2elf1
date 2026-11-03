/*  oggcomp as a library: everything oggcomp.cpp includes up to the
    coroutine, and oc_api.h's functions over it, exported -- every level of
    the program, the level in Init's flags (OC_F_LEVEL).  Built by
    `./mk.sh lib` as oggcomp.so, in the shipping form, or on Windows

        x86_64-w64-mingw32-g++ -O2 -fwrapv -static -shared -DOC_BUILD_DLL \
            -o oggcomp.dll oggcomp_dll.cpp

    for a program of someone else's to drive.  oggcomp itself does not
    load it: it has the levels in it.  */
#ifndef OC_BUILD_DLL
#define OC_BUILD_DLL 1
#endif
#include "oc_platform.inc"
#include "Lib3/coro3b.inc"
#include "Lib3/file_api.inc"
#include "oc_fatal.inc"
#include "ogg_page.inc"
#include "vb_setup.inc"
#include "vb_dict.inc"
#include "oc_rcio.inc"
#include "rc.inc"
#include "cm.inc"
#include "sh_mapping.inc"
#include "tc_base.inc"
#include "tc_ptab.inc"
#include "tc_tables.inc"
#include "tc_fam.inc"
#include "oc_hist.inc"
#include "oc_model.inc"
#include "oc_frame.inc"
#include "oc_header.inc"
#include "oc_raw.inc"
#include "oc_floor.inc"
#include "oc_residue.inc"
#include "vb_packet.inc"
#include "ogg_stream.inc"
#include "oc_coro.inc"
#include "oc_api.inc"
