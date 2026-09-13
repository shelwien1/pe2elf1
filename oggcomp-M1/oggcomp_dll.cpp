/*  The model as a loadable library: everything oggcomp.cpp includes up to
    the coroutine, and oc_api.h's functions over it, exported.  Built by
    `./mk.sh dll N [IDXDIR]` from the IDX model named, in the shipping form,
    as oggcompN.so -- or on Windows

        x86_64-w64-mingw32-g++ -O2 -fwrapv -static -shared -DOC_BUILD_DLL \
            -o oggcompN.dll oggcomp_dll.cpp

    after `./mk.sh mod IDXDIR` has put that model's MOD/ in place.  A
    program picks model N with -N and finds oggcompN.dll (.so) beside
    itself; a stream written through it names N in its header.  */
#ifndef OC_BUILD_DLL
#define OC_BUILD_DLL 1
#endif
#include "oc_platform.inc"
#include "Lib3/coro3b.inc"
#include "Lib3/file_api.inc"
#include "oc_fatal.inc"
#include "ogg_page.inc"
#include "vb_setup.inc"
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
