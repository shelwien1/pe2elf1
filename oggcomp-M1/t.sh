#!/bin/sh
#  Round-trip the test corpus through oggcomp and prove nothing changed.
#
#      ./t.sh                     the bundled corpus, ./oggcomp
#      ./t.sh a.ogg b/*.ogg       those files instead
#      ./t.sh -x build/oggcomp    a binary that is not ./oggcomp
#      ./t.sh -B                  rebuild with ./mk.sh first
#      ./t.sh -k                  keep the .oc and the restored .ogg
#      ./t.sh -1                  one encode per file, no determinism check
#      ./t.sh -n                  skip the refusal tests
#      ./t.sh -v                  pass -v to oggcomp, so it says where the bits went
#
#  What is being tested is a LOSSLESS compressor, so there is one thing that
#  matters and several that are only interesting: `oggcomp d` must give back
#  the input byte for byte.  The ratio, the timings and the throughput are
#  printed because they are what one changes the model for, but a file that
#  compresses beautifully and comes back different is a failed test and a
#  file that expands is a passed one.
#
#  Coding the same file twice must also give the same .oc.  It is the cheap
#  half of the format's contract -- the other half, that the tuning and the
#  shipping build code identically, needs both binaries and lives in
#  `./mk.sh check`, which this script deliberately does not duplicate.  Run
#  both after touching a model:
#
#      ./t.sh && ./mk.sh check
#
#  The refusal tests at the end matter as much as the round trips.  A
#  compressor that cannot reproduce its input exactly has to say so and stop
#  -- silently writing a stream that decodes to something else is the one
#  unforgivable failure -- so the exit status for every kind of bad input is
#  part of the interface, and this checks it, including that a refused run
#  leaves no half-written output behind to be mistaken for a good one.

set -e

here=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
cd "$here"

usage() {
  cat <<'END'
usage: ./t.sh [options] [file.ogg ...]

  -x PATH  test PATH instead of ./oggcomp
  -B       rebuild with ./mk.sh first
  -k       keep each .oc and restored .ogg beside its input
  -1       one encode per file: skip the determinism check
  -n       skip the refusal tests
  -v       pass -v to oggcomp, so it says where the bits went
  -h       this

With no files named, the corpus in testfiles/ is used.
END
}

bin=./oggcomp
build=0
keep=0
twice=1
refusals=1
vflag=

#  Options out, file operands left in "$@" -- rotated to the end one at a
#  time rather than collected into a string, so that a path with a space in
#  it survives being a test file.
argc=$#
while [ $argc -gt 0 ]; do
  case $1 in
    -x) bin=${2:?t.sh: -x wants a path}; shift 2; argc=$((argc - 2));;
    -B) build=1; shift; argc=$((argc - 1));;
    -k) keep=1; shift; argc=$((argc - 1));;
    -1) twice=0; shift; argc=$((argc - 1));;
    -n) refusals=0; shift; argc=$((argc - 1));;
    -v) vflag=-v; shift; argc=$((argc - 1));;
    -h|--help) usage; exit 0;;
    -*) echo "t.sh: no such option: $1" >&2; usage >&2; exit 2;;
    *)  a=$1; shift; set -- "$@" "$a"; argc=$((argc - 1));;
  esac
done

#  A fresh clone has no ./oggcomp.  Build one rather than telling the reader
#  to go and read mk.sh: the tuning build is what mk.sh makes by default, and
#  the tuning build is the one whose parameters are still live, so it is the
#  one a test of a change to them has to run.
if [ $build = 1 ] || { [ "$bin" = ./oggcomp ] && [ ! -x ./oggcomp ]; }; then
  echo "t.sh: building $bin with ./mk.sh"
  ./mk.sh
  echo
fi
[ -x "$bin" ] || { echo "t.sh: $bin is not there -- ./mk.sh builds it" >&2; exit 2; }

#  No files named: the bundled corpus.  The glob is left unexpanded when it
#  matches nothing, which is what the -f test below is looking at.
if [ $# = 0 ]; then
  set -- testfiles/*.ogg
  [ -f "$1" ] || {
    echo "t.sh: no testfiles/*.ogg -- ./testfiles/gen.sh makes them," >&2
    echo "      or name the files to test on the command line" >&2
    exit 2; }
fi
src=$1

tmp=$(mktemp -d "${TMPDIR:-/tmp}/oggcomp-t.XXXXXX")

#  The signal traps exit.  `trap ... EXIT INT TERM` -- one handler for all
#  three -- deletes the scratch directory on Ctrl-C and then lets the script
#  carry on writing into a directory that is no longer there, which for a
#  test script means a run that was interrupted can still print PASS.
trap 'rm -rf "$tmp"' EXIT
trap 'rm -rf "$tmp"; exit 130' INT
trap 'rm -rf "$tmp"; exit 143' TERM

#  Wall time of one run, in seconds, left in $tmp/secs; the command's own
#  exit status comes back as ours.  perl already has to be installed -- it is
#  what turns IDX/ into MOD/ -- and Time::HiRes is core, so timing this way
#  asks for nothing the build did not.  `time` would do as well were its
#  output not a different shape in every shell, and /usr/bin/time is not
#  installed as often as one remembers.
timed() {
  perl -MTime::HiRes=time -e '
    my $out = shift @ARGV;
    my $t0 = time;
    my $rc = system(@ARGV);
    open my $fh, ">", $out or die "$out: $!";
    printf {$fh} "%.3f\n", time() - $t0;
    close $fh;
    exit $rc == -1 ? 127 : ($rc & 127) ? 128 + ($rc & 127) : $rc >> 8;
  ' "$tmp/secs" "$@"
}

n=0; ok=0; same=0; nsame=0
tot_in=0; tot_out=0; tot_enc=0; tot_dec=0
fail=0

printf 'oggcomp: %s\n\n' "$("$bin" 2>&1 | sed -n 1p)"
printf '  %-26s %9s %10s %7s %8s %8s %7s\n' file original compressed ratio enc dec MB/s
printf '  %s\n' '--------------------------------------------------------------------------------'

for f in "$@"; do
  [ -f "$f" ] || { printf '  %-26s %s\n' "$(basename "$f")" 'not there'; fail=1; continue; }
  n=$((n + 1))
  base=$(basename "$f")
  oc=$tmp/a.oc; oc2=$tmp/b.oc; back=$tmp/back.ogg
  rm -f "$oc" "$oc2" "$back"

  note=
  if timed "$bin" c $vflag "$f" "$oc"; then
    enc=$(cat "$tmp/secs")
    if timed "$bin" d $vflag "$oc" "$back"; then
      dec=$(cat "$tmp/secs")
      if cmp -s "$f" "$back"; then
        ok=$((ok + 1))
      else
        note='RESTORED FILE DIFFERS'; fail=1
      fi
    else
      dec=0; note="decode failed (exit $?)"; fail=1
    fi
  else
    enc=0; dec=0; note="encode failed (exit $?)"; fail=1
  fi

  #  The same input twice has to give the same .oc.  If it does not, the
  #  coder is reading something that is not in the file -- uninitialised
  #  memory, a pointer value, the clock -- and the stream it wrote today is
  #  not the stream it will write tomorrow.
  if [ $twice = 1 ] && [ -z "$note" ]; then
    nsame=$((nsame + 1))
    if "$bin" c $vflag "$f" "$oc2" >/dev/null 2>&1 && cmp -s "$oc" "$oc2"; then
      same=$((same + 1))
    else
      note='SECOND ENCODE DIFFERS'; fail=1
    fi
  fi

  in=$(wc -c < "$f")
  tot_in=$((tot_in + in))
  if [ -n "$note" ]; then
    printf '  %-26s %9d  %s\n' "$base" "$in" "$note"
  else
    out=$(wc -c < "$oc")
    tot_out=$((tot_out + out))
    tot_enc=$(awk -v a="$tot_enc" -v b="$enc" 'BEGIN{printf "%.3f", a+b}')
    tot_dec=$(awk -v a="$tot_dec" -v b="$dec" 'BEGIN{printf "%.3f", a+b}')
    #  printf(...) with the parentheses, not `printf ...`: inside a bare awk
    #  print list a `>` is a redirection, so the throughput guard below would
    #  quietly write the row to a file called "0" instead of to the terminal.
    awk -v f="$base" -v i="$in" -v o="$out" -v e="$enc" -v d="$dec" 'BEGIN{
      printf("  %-26s %9d %10d %6.2f%% %7.2fs %7.2fs %7.2f\n",
             f, i, o, 100.0*o/i, e, d, (e > 0 ? i/e/1048576.0 : 0)) }'
  fi

  if [ $keep = 1 ]; then
    cp -f "$oc" "$f.oc" 2>/dev/null || true
    cp -f "$back" "$f.restored" 2>/dev/null || true
  fi
done

printf '  %s\n' '--------------------------------------------------------------------------------'
[ "$n" = 1 ] && label="1 file" || label="$n files"
awk -v l="$label" -v i="$tot_in" -v o="$tot_out" -v e="$tot_enc" -v d="$tot_dec" 'BEGIN{
  printf("  %-26s %9d %10d %6.2f%% %7.2fs %7.2fs %7.2f\n",
         l, i, o, (i > 0 ? 100.0*o/i : 0), e, d, (e > 0 ? i/e/1048576.0 : 0)) }'
echo

#  Every way of being told no.  The exit status is the interface here: 1 is
#  "this input is not something I can code back exactly", 2 is "you typed it
#  wrong", 3 is "the filesystem said no" -- and a run that ends in any of
#  them must not leave an output file, because a half-written .oc that looks
#  like a whole one is how a backup turns out to be nothing.
bad_refusal=0; nrefusal=0

#  Several of the tests below want a good .oc to damage, and take it from
#  the first file under test.  If even that will not code then the round
#  trips above have already failed and there is nothing here left to learn.
if [ $refusals = 1 ] && ! "$bin" c "$src" "$tmp/good.oc" >/dev/null 2>&1; then
  echo "t.sh: refusal tests skipped -- $src does not code" >&2
  refusals=0
fi

if [ $refusals = 1 ]; then
  : > "$tmp/empty.ogg"
  head -c 400 "$src" > "$tmp/cut.ogg"
  head -c 200 "$tmp/good.oc" > "$tmp/cut.oc"
  cp "$tmp/good.oc" "$tmp/ver.oc"
  printf '\177' | dd of="$tmp/ver.oc" bs=1 seek=5 count=1 conv=notrunc >/dev/null 2>&1

  refuse() {  # $1 what, $2 wanted exit status, rest: arguments to the binary
    what=$1; want=$2; shift 2
    nrefusal=$((nrefusal + 1))
    rm -f "$tmp/no.out"
    set +e
    "$bin" "$@" >"$tmp/msg" 2>&1
    got=$?
    set -e
    if [ "$got" != "$want" ]; then
      printf '  %-26s exit %d, wanted %d\n' "$what" "$got" "$want"
      bad_refusal=$((bad_refusal + 1))
    elif [ -e "$tmp/no.out" ]; then
      printf '  %-26s exit %d, but left an output file behind\n' "$what" "$got"
      bad_refusal=$((bad_refusal + 1))
    else
      #  The scratch directory's name is longer than what it is worth
      #  saying, and it is different on every run, so take it out of the
      #  message before showing it.
      printf '  %-26s exit %d  %s\n' "$what" "$got" \
             "$(sed -n 1p "$tmp/msg" |
                sed "s|$tmp/||g; s|testfiles/refused/||g; s|^[^ ]*: ||" |
                cut -c1-48)"
    fi
  }

  printf '  %s\n' 'refusals'
  refuse 'empty input'        1 c "$tmp/empty.ogg" "$tmp/no.out"
  refuse 'truncated ogg'      1 c "$tmp/cut.ogg"   "$tmp/no.out"
  refuse 'not an ogg at all'  1 c "$tmp/good.oc"   "$tmp/no.out"
  refuse 'ogg given to d'     1 d "$src"           "$tmp/no.out"
  refuse 'truncated .oc'      1 d "$tmp/cut.oc"    "$tmp/no.out"
  refuse 'wrong version byte' 1 d "$tmp/ver.oc"    "$tmp/no.out"
  refuse 'input not there'    3 c "$tmp/nothing"   "$tmp/no.out"
  refuse 'output over input'  2 c "$src"           "$src"
  refuse 'no arguments'       2
  refuse 'unknown mode'       2 z "$src"           "$tmp/no.out"
  refuse 'unknown option'     2 c -Z "$src"        "$tmp/no.out"

  #  testfiles/refused/ holds one file per way of being an Ogg that cannot
  #  be given back byte for byte -- a bad page CRC, a stream with no
  #  end-of-stream page, an ID3 tag in front of the first page, junk after
  #  the last one, and an Ogg that is Opus rather than Vorbis.  Each must
  #  still be refused.  A build that starts ACCEPTING one of these is the
  #  failure this whole directory exists to catch: it would be writing a
  #  stream whose decode is not the file it was given.
  if [ -d testfiles/refused ]; then
    for r in testfiles/refused/*; do
      [ -f "$r" ] || continue
      refuse "$(basename "$r")" 1 c "$r" "$tmp/no.out"
    done
  fi
  echo
fi

#  A lone `-` is a path meaning stdin on the way in and stdout on the way
#  out, which is not a thing one can tell from the usage text, and the only
#  part of the interface with no file behind it.
if [ -f "$src" ]; then
  printf '  %s\n' 'stdin and stdout'
  if "$bin" c - "$tmp/pipe.oc" < "$src" 2>/dev/null &&
     "$bin" d "$tmp/pipe.oc" - > "$tmp/pipe.ogg" 2>/dev/null &&
     cmp -s "$src" "$tmp/pipe.ogg"; then
    printf '  %-26s %s\n' 'c - out.oc < in.ogg' 'round-tripped through the pipe'
  else
    printf '  %-26s %s\n' 'c - out.oc < in.ogg' 'FAILED'
    fail=1
  fi
  echo
fi

printf 't.sh: %d/%d round-tripped byte for byte' "$ok" "$n"
[ $twice = 1 ] && printf ', %d/%d coded identically twice' "$same" "$nsame"
echo
[ $refusals = 1 ] &&
  printf 't.sh: %d/%d refusals gave the exit status they should\n' \
         "$((nrefusal - bad_refusal))" "$nrefusal"

if [ $fail = 0 ] && [ $bad_refusal = 0 ] && [ $n -gt 0 ]; then
  echo "t.sh: PASS"
else
  echo "t.sh: FAIL" >&2
  exit 1
fi
