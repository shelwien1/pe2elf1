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
      printf "  OK   %-10s %s\n" "$1" "`basename $c`"
    else
      printf "  FAIL %-10s %s\n" "$1" "`basename $c`"; fails=`expr $fails + 1`
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

  # How many textures came out.  A mis-sized extent still round-trips -- the
  # marker records exactly what was taken -- so the container comparison above
  # cannot see one; compare the sidecars against the real files to check that.
  printf "  --   %-10s %s carved\n" "sidecars" "`ls -1 "$T"/s/p_*.dds 2>/dev/null | wc -l`"
done
rm -rf "$T"
if [ $fails -eq 0 ]; then echo "all round trips OK"; else echo "$fails FAILURES"; exit 1; fi
