#!/bin/sh
# t_ddsdet.sh [container ...] -- carve/restore round-trip check for ddsdet.
#
# For each container given (default dds_det_fail.tar) every direction pair is
# run and the restored container is compared byte for byte with the original:
#
#   c / d          sidecar .dds per texture, markers in the container
#   c --zero / d   markers padded back to the texture length
#   cs / ds        one .d5s archive + an uncompressed restfile
#   csn / dsn      same, model reset per .dds file
#   csa / dsa      --all: the container itself is coded too, one output file
#   csan / dsan    --all, non-solid
#   csb / dsb      endpoints to .bmp instead of the coder
#   cp / d         --dedup: repeats reference the first copy's sidecar
#   csap / dsa     --dedup inside the archive
#   csbp / dsb     --dedup alongside external endpoints
#
# The dedup runs are repeated at a 1 MiB budget and 256 slots, which on any
# real container forces constant eviction and bucket collisions.  That is the
# case worth testing: the encoder may only reference an entry the decoder still
# holds, so the two must evict in lockstep, and a divergence shows up here as a
# failed compare rather than as a rare wrong byte much later.
#
# Nothing here inspects the archive: the only thing that makes a carver correct
# is that the bytes come back, so that is what is asserted.
set -e
DD=./ddsdet
T=${TMPDIR:-/tmp}/ddsdet_t.$$
trap 'rm -rf "$T"' EXIT INT TERM
[ $# -gt 0 ] || set -- dds_det_fail.tar

fails=0
for c in "$@"; do
  rm -rf "$T"; mkdir -p "$T/s" "$T/z" "$T/b"

  chk() {                      # chk <label> <restored file>
    if cmp -s "$c" "$2"; then
      printf "  OK   %-16s %s\n" "$1" "`basename $c`"
    else
      printf "  FAIL %-16s %s\n" "$1" "`basename $c`"; fails=`expr $fails + 1`
    fi
  }

  $DD c        "$c" "$T/c.rest" "$T/s/p_" >/dev/null 2>&1
  $DD d        "$T/c.rest" "$T/c.out" "$T/s/p_" >/dev/null 2>&1
  chk "c/d" "$T/c.out"

  $DD c --zero "$c" "$T/z.rest" "$T/z/p_" >/dev/null 2>&1
  $DD d        "$T/z.rest" "$T/z.out" "$T/z/p_" >/dev/null 2>&1
  chk "c-zero/d" "$T/z.out"

  $DD cs   "$c" "$T/s.rest" "$T/s.d5s" >/dev/null 2>&1
  $DD ds   "$T/s.rest" "$T/s.d5s" "$T/s.out" >/dev/null 2>&1
  chk "cs/ds" "$T/s.out"

  $DD csn  "$c" "$T/n.rest" "$T/n.d5s" >/dev/null 2>&1
  $DD dsn  "$T/n.rest" "$T/n.d5s" "$T/n.out" >/dev/null 2>&1
  chk "csn/dsn" "$T/n.out"

  $DD csa  "$c" "$T/a.d5s" >/dev/null 2>&1
  $DD dsa  "$T/a.d5s" "$T/a.out" >/dev/null 2>&1
  chk "csa/dsa" "$T/a.out"

  $DD csan "$c" "$T/an.d5s" >/dev/null 2>&1
  $DD dsan "$T/an.d5s" "$T/an.out" >/dev/null 2>&1
  chk "csan/dsan" "$T/an.out"

  $DD csb  "$c" "$T/b.rest" "$T/b/e_" >/dev/null 2>&1
  $DD dsb  "$T/b.rest" "$T/b/e_" "$T/b.out" >/dev/null 2>&1
  chk "csb/dsb" "$T/b.out"

  # --- dedup, at the default geometry and then at one that thrashes ---------
  # thrashing geometry first, so the sidecar count printed below is the one the
  # default geometry produced
  for g in "--dedup-mem 1048576 --dedup-slots 256" ""; do
    [ -z "$g" ] && tag="" || tag="+small"
    rm -rf "$T/p" "$T/pb"; mkdir -p "$T/p" "$T/pb"

    $DD cp $g "$c" "$T/p.rest" "$T/p/p_" >/dev/null 2>&1
    $DD d     "$T/p.rest" "$T/p.out" "$T/p/p_" >/dev/null 2>&1
    chk "cp/d$tag" "$T/p.out"

    $DD csap $g "$c" "$T/ap.d5s" >/dev/null 2>&1
    $DD dsa     "$T/ap.d5s" "$T/ap.out" >/dev/null 2>&1
    chk "csap/dsa$tag" "$T/ap.out"

    $DD csp  $g "$c" "$T/sp.rest" "$T/sp.d5s" >/dev/null 2>&1
    $DD ds      "$T/sp.rest" "$T/sp.d5s" "$T/sp.out" >/dev/null 2>&1
    chk "csp/ds$tag" "$T/sp.out"

    $DD csbp $g "$c" "$T/pb.rest" "$T/pb/e_" >/dev/null 2>&1
    $DD dsb     "$T/pb.rest" "$T/pb/e_" "$T/pb.out" >/dev/null 2>&1
    chk "csbp/dsb$tag" "$T/pb.out"
  done

  # --zero and dedup together: the placeholder length is per occurrence, the
  # marker index is per unique texture, and the two must not get confused.
  rm -rf "$T/zp"; mkdir -p "$T/zp"
  $DD cp --zero "$c" "$T/zp.rest" "$T/zp/p_" >/dev/null 2>&1
  $DD d         "$T/zp.rest" "$T/zp.out" "$T/zp/p_" >/dev/null 2>&1
  chk "cp-zero/d" "$T/zp.out"

  # How many textures came out.  A mis-sized extent still round-trips -- the
  # marker records exactly what was taken -- so the container comparison above
  # cannot see one; compare the sidecars against the real files to check that.
  printf "  --   %-12s %s carved, %s of them distinct\n" "sidecars" \
         "`ls -1 "$T"/s/p_*.dds 2>/dev/null | wc -l`" \
         "`ls -1 "$T"/p/p_*.dds 2>/dev/null | wc -l`"
done
rm -rf "$T"
if [ $fails -eq 0 ]; then echo "all round trips OK"; else echo "$fails FAILURES"; exit 1; fi
