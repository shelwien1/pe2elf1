#!/usr/bin/env bash
# test.sh — the round-trip gate, against a native binary or a Windows one.
#
#   ./test.sh ./bmf                 native
#   ./test.sh --wine ./bmf.exe      under wine
#
# For every image in testfiles/:
#
#   bmf c <image>.bmp <image>.bmf
#   bmf d <image>.bmf <image>.bmp
#
# and the recovered file has to be byte-identical to the input, whole file.
# There is one compression mode now -- the binary always uses -S -Q9 -- so
# there is nothing to sweep: this is the mode.
#
# Next to each image is its reference stream — testfiles/ref_t24.bmf for
# testfiles/t24.bmp — and the compressed output has to match it byte for byte.
# That is the check that makes this a refactoring gate rather than a smoke test:
# a round-trip only says the code is self-consistent, while the reference says
# the code still encodes the way it did before you touched it.  A change that
# moves a stream is a change that altered behaviour, and has to be justified by
# regenerating the references deliberately (tools/mkrefs.sh) rather than by
# noticing later.
#
# BMF_TESTDIR and BMF_IMAGES override the corpus.  BMF_NOREF=1 skips the
# reference check, for the one case it is meant for: producing new references.
set -u
cd "$(dirname "$0")"

TESTDIR=${BMF_TESTDIR:-testfiles}
WORK=run

RUN=""
if [ "${1:-}" = "--wine" ]; then
  shift
  RUN=${WINE:-$(command -v wine || echo /usr/lib/wine/wine)}
  [ -x "$RUN" ] || { echo "no wine (Debian/Ubuntu: apt install wine wine32)"; exit 1; }
  export WINEDEBUG=${WINEDEBUG:--all}
  : "${WINEPREFIX:=$PWD/.wine}"
  export WINEPREFIX
fi
BIN=${1:-./bmf}
[ -x "$BIN" ] || { echo "no $BIN (run ./build.sh)"; exit 1; }

[ -d "$TESTDIR" ] || { echo "no test corpus at $TESTDIR"; exit 1; }
# Default corpus: every .bmp in TESTDIR, in a stable order.  `out_<name>.bmp`
# is not an input — it is what the decoder is expected to write for an input
# it does not reproduce byte for byte (see the comparison below).
IMAGES=${BMF_IMAGES:-$(cd "$TESTDIR" && ls *.bmp 2>/dev/null | grep -v '^out_' | sort)}
[ -n "$IMAGES" ] || { echo "no .bmp images in $TESTDIR"; exit 1; }

rm -rf "$WORK"; mkdir -p "$WORK"
cp "$BIN" "$WORK/" || exit 1
BIN="./$(basename "$BIN")"
for img in $IMAGES; do
  [ -f "$TESTDIR/$img" ] || { echo "missing test image: $TESTDIR/$img"; exit 1; }
  cp "$TESTDIR/$img" "$WORK/orig_$img"
done

fail=0
for img in $IMAGES; do
  st="${img%.bmp}"
  ref="$TESTDIR/ref_$st.bmf"
  (
    cd "$WORK"
    timeout "${BMF_TIMEOUT:-300}" $RUN "$BIN" c "orig_$img" "$st.bmf" >"$st.compress.log" 2>&1
    rc=$?; [ $rc -ne 0 ] && { echo "$st: COMPRESS FAILED (rc=$rc)"; cat "$st.compress.log"; exit 1; }
    [ -s "$st.bmf" ] || { echo "$st: NO STREAM PRODUCED"; exit 1; }
    if [ "${BMF_NOREF:-0}" != 1 ]; then
      if [ -f "../$ref" ]; then
        cmp -s "$st.bmf" "../$ref" || {
          echo "$st: STREAM CHANGED ($(stat -c%s "$st.bmf") bytes, reference is $(stat -c%s "../$ref"))"
          exit 1
        }
      else
        echo "$st: NO REFERENCE STREAM ($ref) — run tools/mkrefs.sh"
        exit 1
      fi
    fi
    timeout "${BMF_TIMEOUT:-300}" $RUN "$BIN" d "$st.bmf" "$img" >"$st.decompress.log" 2>&1
    rc=$?; [ $rc -ne 0 ] && { echo "$st: DECOMPRESS FAILED (rc=$rc)"; cat "$st.decompress.log"; exit 1; }
    # Most inputs come back byte for byte.  An RLE-compressed BMP does not:
    # BMF decodes the runs on the way in and re-encodes them on the way out,
    # with its own run splitting, so the pixels are identical and the file is
    # not.  For those, testfiles/out_<name>.bmp holds what the decoder is
    # expected to write, which still catches any change in behaviour.
    want="orig_$img"
    [ -f "../$TESTDIR/out_$img" ] && want="../$TESTDIR/out_$img"
    cmp -s "$want" "$img" || { echo "$st: NOT LOSSLESS"; exit 1; }
    printf '%-12s ok  %8s -> %8s\n' "$st" \
      "$(stat -c%s "orig_$img")" "$(stat -c%s "$st.bmf")"
    exit 0
  ) || fail=1
done
# The archive holds more than one member.  `bmf c` appends, `bmf d` walks every
# member and decodes each one, and the writing half of that was removed once by
# a change of the output mode that nothing caught, because no test built an
# archive with two images in it.  This does.
if [ "${BMF_ARCHIVE:-1}" = 1 ] && [ -f "$TESTDIR/t1.bmp" ] && [ -f "$TESTDIR/t8g.bmp" ]; then
  (
    cd "$WORK"
    rm -f arc.bmf arc.bmp
    $RUN "$BIN" c orig_t1.bmp arc.bmf >arc.log 2>&1 &&
    $RUN "$BIN" c orig_t8g.bmp arc.bmf >>arc.log 2>&1 ||
      { echo "archive: COMPRESS FAILED"; cat arc.log; exit 1; }
    one=$(stat -c%s t8g.bmf) both=$(stat -c%s arc.bmf)
    [ "$both" -gt "$one" ] || {
      echo "archive: SECOND MEMBER REPLACED THE FIRST ($both bytes, t8g alone is $one)"
      exit 1
    }
    $RUN "$BIN" d arc.bmf arc.bmp >arc.d.log 2>&1 ||
      { echo "archive: DECOMPRESS FAILED"; cat arc.d.log; exit 1; }
    grep -q 'number: 2' arc.d.log || {
      echo "archive: SECOND MEMBER NOT READ BACK"; cat arc.d.log; exit 1
    }
    cmp -s orig_t8g.bmp arc.bmp || { echo "archive: LAST MEMBER NOT LOSSLESS"; exit 1; }
    printf '%-12s ok  %8s -> %8s  (2 members)\n' archive \
      "$(stat -c%s orig_t1.bmp)" "$both"
    exit 0
  ) || fail=1
fi

# Input the program is expected to refuse.  Every check above hands it a file
# it can read, so the whole of the error half -- the "bad file!", "Read error!"
# and "Can't open file:" exits, and the frees that unwind to them -- ran only in
# whatever way a passing run happens to reach it, which is not at all.  These
# runs are the cheapest coverage left, and they check the thing worth checking
# about an error path: that it is an exit and not a crash.
#
# The exit status is pinned rather than merely required to be nonzero.  A status
# is behaviour the same way a stream is, and these are the runs that decide
# which one you get; leaving them as "not zero" would let 3 and 4 trade places
# unnoticed.  A status of 124 is the timeout and anything at or above 128 is a
# fatal signal, so those can never be an expectation here.
#
# The empty file is the one case that is not an error: an archive is a sequence
# of members and a file with none in it decodes to nothing, successfully.
if [ "${BMF_MALFORMED:-1}" = 1 ]; then
  (
    cd "$WORK"
    # The largest stream, not the first.  Taking the first made the truncation
    # lengths depend on the corpus: under BMF_IMAGES with only small images,
    # `head -c 40000` of a 192-byte stream is the whole file, which decodes
    # perfectly and fails a check that expects a refusal.  A cut is only a cut
    # if there is something past it, so the lengths are filtered against the
    # size as well.
    ref=$(ls -S -- *.bmf 2>/dev/null | grep -v '^arc\.' | head -1)
    # The largest *uncompressed* BMP -- the compression field of the info
    # header, at offset 30, has to be 0.  This used to be here because cutting
    # an RLE8 file short produced a buffer overflow rather than a refusal, and
    # the case with an exit to check was the uncompressed one.  Both are
    # refusals now (REFACTORING.md §6) and the RLE cut is one of the cases
    # below; this still picks an uncompressed image because the two truncations
    # fail in different places and both are worth having.
    img=""
    for f in $(ls -S -- orig_*.bmp 2>/dev/null); do
      [ "$(od -An -tu4 -j30 -N4 "$f" | tr -d ' ')" = 0 ] || continue
      img=$f; break
    done
    [ -n "$ref" ] && [ -n "$img" ] || { echo "malformed: no stream to truncate"; exit 1; }
    : >empty.bmf
    head -c 2000 /dev/zero >zeros.bmf
    head -c 2000 /dev/zero | tr '\0' '\377' >ones.bmf
    cuts=""
    for n in 4 40 400 4000 40000; do
      [ "$n" -lt "$(stat -c%s "$ref")" ] || continue
      head -c $n "$ref" >"cut$n.bmf"; cuts="$cuts $n"
    done
    head -c 6000 "$img" >cut.bmp

    # The seven that used to crash.  Each was recorded in REFACTORING.md §6 or
    # found by fuzzing the header, and each is now a refusal with an exit to
    # check -- which is the only way a fix stays fixed.
    rle=$(for f in $(ls -S -- orig_*.bmp 2>/dev/null); do
            [ "$(od -An -tu4 -j30 -N4 "$f" | tr -d ' ')" = 0 ] || { echo "$f"; break; }
          done)
    [ -n "$rle" ] && head -c 6000 "$rle" >cutrle.bmp
    # A top-down BMP: a negative height, legal and common.
    hdr_patch() {   # file offset value width -> writes a little-endian field
      python3 -c 'import struct,sys
d=bytearray(open(sys.argv[1],"rb").read())
struct.pack_into("<i" if sys.argv[4]=="i" else "<I", d, int(sys.argv[2]), int(sys.argv[3]))
open(sys.argv[5],"wb").write(d)' "$1" "$2" "$3" "$4" "$5"
    }
    # `biClrUsed` only means anything at eight bits and below, so that case
    # needs a paletted image and not just the largest one.
    pal=$(for f in $(ls -S -- orig_*.bmp 2>/dev/null); do
            [ "$(od -An -tu4 -j30 -N4 "$f" | tr -d ' ')" = 0 ] || continue
            [ "$(od -An -tu2 -j28 -N2 "$f" | tr -d ' ')" -le 8 ] || continue
            echo "$f"; break
          done)
    hdr_patch "$img" 22 -240 i topdown.bmp
    [ -n "$pal" ] && hdr_patch "$pal" 46 25600 I clrused.bmp
    hdr_patch "$img" 18 32960 i wide.bmp
    hdr_patch "$img" 28 16 I bpp16.bmp
    # An output that exists and is not an archive: `compress_image` walks it to
    # append, and used to write through the FILE `expand_image` had closed.
    echo "not an archive" >notarc.out
    rm -f gone.bmp gone.bmf

    bad=0 cases=0
    # mode  input        want  what it should hit
    while read -r mode file want _; do
      [ -n "${mode:-}" ] || continue
      rm -f mal.out
      timeout "${BMF_TIMEOUT:-300}" $RUN "$BIN" $mode "$file" mal.out >mal.log 2>&1
      rc=$?
      if [ "$rc" != "$want" ]; then
        echo "malformed: $BIN $mode $file exited $rc, expected $want"
        cat mal.log; bad=1; continue
      fi
      [ -f mal.out ] && { echo "malformed: $BIN $mode $file wrote an output anyway"; bad=1; }
      cases=$((cases + 1))
    done <<CASES
d empty.bmf   0   an archive with no members in it
$(for n in $cuts; do echo "d cut$n.bmf 3 cut at $n bytes"; done)
d zeros.bmf   3   a stream that is not one
d ones.bmf    3   the other end of the same
d gone.bmf    6   an input that is not there
c gone.bmp    6   the same, compressing
c cut.bmp     4   a BMP whose pixels run out
$([ -n "$rle" ] && echo "c cutrle.bmp 4 an RLE BMP that ends mid-run")
c topdown.bmp 4   a top-down BMP, which this reader does not do
$([ -n "$pal" ] && echo "c clrused.bmp 4 biClrUsed past the palette alloc_image reserves")
c wide.bmp    4   a row wider than the sixteen bits the stride has
c bpp16.bmp   4   a depth the writer cannot put back in a BMP
CASES

    # The two files that are each other's wrong kind.  Named separately because
    # they come from the corpus rather than from anything written above.
    for pair in "d $img 3" "c $ref 4"; do
      set -- $pair
      rm -f mal.out
      timeout "${BMF_TIMEOUT:-300}" $RUN "$BIN" "$1" "$2" mal.out >mal.log 2>&1
      rc=$?
      [ "$rc" = "$3" ] || { echo "malformed: $BIN $1 $2 exited $rc, expected $3"; cat mal.log; bad=1; }
      cases=$((cases + 1))
    done

    # An output that already exists and cannot be walked to its end.
    # `compress_image` walks an archive to append after it; `expand_image`
    # closes the file and nulls `arc->fp` on the first member it cannot parse,
    # and the walk used to fall through to `fwrite(..., nullptr)`.
    #
    # A file that is not an archive *at all* does not reach that -- the walk in
    # `bmf_open_archive` refuses it first, with exit 3.  What does is a real
    # archive with something after the last member, which is what an interrupted
    # write leaves behind.  The output is named here rather than `mal.out`,
    # because being the output is the whole case.
    for want in "notarc.out 3" "tail.bmf 5"; do
      set -- $want
      rm -f "$1"
      [ "$1" = notarc.out ] && echo "not an archive" >"$1"
      [ "$1" = tail.bmf ] && {
        timeout "${BMF_TIMEOUT:-300}" $RUN "$BIN" c "$img" "$1" >/dev/null 2>&1
        printf 'garbagegarbage' >>"$1"
      }
      timeout "${BMF_TIMEOUT:-300}" $RUN "$BIN" c "$img" "$1" >mal.log 2>&1
      rc=$?
      [ "$rc" = "$2" ] || { echo "malformed: $BIN c $img $1 exited $rc, expected $2"
                            cat mal.log; bad=1; }
      cases=$((cases + 1))
    done

    # No arguments at all, and a mode letter that is neither c nor d.
    for args in "" "x a b"; do
      timeout "${BMF_TIMEOUT:-300}" $RUN "$BIN" $args >mal.log 2>&1
      rc=$?
      [ "$rc" = 1 ] || { echo "malformed: $BIN $args exited $rc, expected 1"; cat mal.log; bad=1; }
      cases=$((cases + 1))
    done

    [ $bad -eq 0 ] || exit 1
    # Counted, not computed: the arithmetic here said `cuts + 10` and went on
    # saying 15 while five cases were added under it.
    printf '%-12s ok  refused %d inputs, no crash\n' malformed "$cases"
    exit 0
  ) || fail=1
fi

# An allocation that fails.  `main` installs out_of_memory_handler, which prints
# "Out of memory!" and exits 7, and for a while nothing could call it: the
# bodies' `operator new` had been rewritten to a bare `malloc`, so a run that
# could not get memory dereferenced null instead.  `ulimit -v` reproduces that
# in a second, which is the whole reason it went unnoticed for as long as it did.
#
# The limit is not a fixed number.  How much address space the binary needs
# before it starts depends on the libc and on whether it is static, so this
# tries a ladder and asks two things of it: no limit may kill the program with a
# signal, and at least one must produce the diagnostic.  A limit low enough to
# stop the loader is not interesting and shows up as neither.
#
# Skipped under wine, where the limit governs the emulator and not the program.
if [ "${BMF_OOM:-1}" = 1 ] && [ -z "$RUN" ] &&
   ( ulimit -v 65536 ) >/dev/null 2>&1; then
  (
    cd "$WORK"
    img=$(ls -- orig_*.bmp 2>/dev/null | head -1)
    reported=0 bad=0
    for kb in 6000 8000 10000 12000 16000; do
      rm -f oom.bmf
      ( ulimit -v $kb; timeout "${BMF_TIMEOUT:-300}" $RUN "$BIN" c "$img" oom.bmf ) \
        >oom.log 2>&1
      rc=$?
      case $rc in
        7) grep -q 'Out of memory!' oom.log || {
             echo "oom: -v $kb exited 7 without saying why"; cat oom.log; bad=1; }
           reported=1 ;;
        0|1) ;;                       # enough memory, or too little to start
        *) echo "oom: -v $kb exited $rc, not 7"; cat oom.log; bad=1 ;;
      esac
    done
    [ $reported = 1 ] || { echo "oom: no limit in the ladder made an allocation fail"; bad=1; }
    [ $bad -eq 0 ] || exit 1
    printf '%-12s ok  reports and exits 7\n' 'out of mem'
    exit 0
  ) || fail=1
fi

# The strict build, which is a property of the source and not of any stream, so
# nothing above can see it drift.  REFACTORING3.md lost its zero twice and found
# out late both times; REFACTORING4.md §8 says it belongs in the gate, so here
# it is.  BMF_STRICT=0 skips it -- for a tree that is mid-refactor on purpose.
if [ "${BMF_STRICT_GATE:-1}" = 1 ] && [ -x ./build.sh ]; then
  n=$(BMF_STRICT=1 ./build.sh >/dev/null 2>&1; grep -c 'error:' strict.log)
  if [ "$n" = 0 ]; then
    printf '%-12s ok  0 conversions need -fpermissive\n' 'strict'
  else
    echo "strict: $n conversions need -fpermissive (see strict.log)"; fail=1
  fi
fi

# The conversion scoreboard, REFACTORING5.md §2.5.  A ratchet and not a target:
# `warn.txt` holds what the tree costs today and this fails when the count goes
# up.  Lowering `warn.txt` is how a round records that it took some away; every
# warning left is meant to have a reason beside it, because a decompilation
# reinterprets memory on purpose and some of these are the program.
if [ "${BMF_WARN_GATE:-1}" = 1 ] && [ -x ./build.sh ] && [ -r warn.txt ]; then
  want=$(cat warn.txt)
  got=$(BMF_WARN=1 ./build.sh 2>/dev/null | tail -1)
  if [ "$got" -le "$want" ] 2>/dev/null; then
    printf '%-12s ok  %s conversion warnings, ceiling %s\n' 'warnings' "$got" "$want"
  else
    echo "warnings: $got, up from the $want in warn.txt (see warn.log)"; fail=1
  fi
fi

[ $fail -eq 0 ] || { echo "FAIL"; exit 1; }
echo "PASS"
