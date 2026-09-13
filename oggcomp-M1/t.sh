#!/bin/sh
#  Round-trip the test corpus through oggcomp and prove nothing changed.
#
#      ./t.sh                     the bundled corpus, ./oggcomp
#      ./t.sh a.ogg b/*.ogg       those files instead
#      ./t.sh -x build/oggcomp    a binary that is not ./oggcomp
#      ./t.sh -B                  rebuild with ./mk.sh first, oggcomp1.so too
#      ./t.sh -k                  keep the .oc and the restored .ogg
#      ./t.sh -1                  one encode per file, no determinism check
#      ./t.sh -n                  skip the tests after the table
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
#  The two sections after the table matter as much as the round trips.
#  `oggcomp c` takes any file -- an Ogg with a tag in front, a stream cut
#  off halfway, a file that is not Ogg at all -- and gives it back byte for
#  byte, so the first section feeds it things that are not Vorbis and
#  requires exactly that.  What it can still refuse is a `.oc` it did not
#  write and a command line it cannot read, and the exit status for each of
#  those is part of the interface, so the second section checks it,
#  including that a refused run leaves no half-written output behind to be
#  mistaken for a good one.
#  The third section is oggdet, the carver built beside the compressor: a
#  container of streams and other bytes has to come back whole, with and
#  without -c, and the segment -c writes for a stream has to be the file
#  the compressor writes for that stream alone.  The fourth is the model
#  as a library: oggcomp1.so, built from the tree's own IDX by `./mk.sh
#  dll 1`, loaded by -1, has to code what -0 codes, byte for byte but for
#  the model's number in the header.

set -e

here=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
cd "$here"

usage() {
  cat <<'END'
usage: ./t.sh [options] [file.ogg ...]

  -x PATH  test PATH instead of ./oggcomp
  -B       rebuild with ./mk.sh first, and oggcomp1.so with ./mk.sh dll 1
  -k       keep each .oc and restored .ogg beside its input
  -1       one encode per file: skip the determinism check
  -n       skip the tests after the table: anything round-trips, refusals, oggdet
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
if [ $build = 1 ]; then
  echo "t.sh: building oggcomp1.so with ./mk.sh dll 1"
  ./mk.sh dll 1
  echo
fi
[ -x "$bin" ] || { echo "t.sh: $bin is not there -- ./mk.sh builds it" >&2; exit 2; }

#  No files named: the bundled corpus, the .bin files included -- they are
#  the inputs that are not Ogg at all.  A glob that matches nothing is left
#  unexpanded, which the -f test drops.
if [ $# = 0 ]; then
  for g in testfiles/*.ogg testfiles/*.bin; do
    [ -f "$g" ] && set -- "$@" "$g"
  done
  [ $# -gt 0 ] || {
    echo "t.sh: no testfiles/*.ogg -- ./testfiles/gen.sh makes them," >&2
    echo "      or name the files to test on the command line" >&2
    exit 2; }
fi
src=$1

#  Two process limits turn every row of the table below into the same
#  failure.  Measured on this tree: the model tables and the static pools
#  put VmPeak at 858 MB -- reserved, barely any of it resident -- so a
#  `ulimit -v` under about that makes every run exit 3, and the coroutine's
#  stack pad needs 288 kB,
#  under which the failure is a bare SIGSEGV with nothing on stderr at all.
#  A warning here beats a column of "encode failed (exit 139)" with no
#  hint of why.  A warning, not a refusal: a build with smaller tables is a
#  thing someone may be measuring.
lim=$(ulimit -v 2>/dev/null) || lim=unlimited
case $lim in
  ''|unlimited) ;;
  *[!0-9]*) ;;
  *) [ "$lim" -ge 900000 ] || echo "t.sh: ulimit -v is $lim kB and the model" \
       "tables reserve 858 MB of address space -- every run will exit 3" >&2;;
esac
lim=$(ulimit -s 2>/dev/null) || lim=unlimited
case $lim in
  ''|unlimited) ;;
  *[!0-9]*) ;;
  *) [ "$lim" -ge 288 ] || echo "t.sh: ulimit -s is $lim kB and oggcomp wants 288 --" \
       "every run will be killed by SIGSEGV, silently" >&2;;
esac

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
    #  system with the explicit indirect object never reaches a shell, not
    #  even when the list has come down to one element.
    my $rc = system { $ARGV[0] } @ARGV;
    open my $fh, ">", $out or die "$out: $!";
    printf {$fh} "%.3f\n", time() - $t0;
    close $fh;
    #  Parenthesised, all of it: `exit` is a named unary operator, so
    #  `exit $rc == -1 ? ...` is `(exit $rc) == -1 ? ...` -- it exits with
    #  $rc, which for a child that exited 5 is 5<<8, which exit truncates
    #  to 0.  Every failing run then looked like a successful one.
    exit(($rc == -1) ? 127 : ($rc & 127) ? 128 + ($rc & 127) : ($rc >> 8));
  ' "$tmp/secs" "$@"
}

n=0; ok=0; same=0; nsame=0; counted=0; srcok=0
tot_in=0; tot_out=0; tot_enc=0; tot_dec=0
fail=0

printf 'oggcomp: %s\n\n' "$("$bin" 2>&1 | sed -n 1p)"
printf '  %-26s %9s %10s %7s %8s %8s %7s\n' file original compressed ratio enc dec MB/s
printf '  %s\n' '--------------------------------------------------------------------------------'

#  One file at a time.  Not for memory -- the tables are reserved, not
#  resident, and the whole corpus peaks at some 35 MB of RSS -- but because
#  the enc, dec and MB/s columns are wall-clock, and numbers taken under
#  contention are worse than no numbers.  Serial, the corpus takes 5s.
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
      #  $? first: the assignment beside it would be what it reported.
      rc=$?; dec=0; note="decode failed (exit $rc)"; fail=1
    fi
  else
    rc=$?; enc=0; dec=0; note="encode failed (exit $rc)"; fail=1
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

  #  Whether the file the two tests after the table work from came back.
  #  If it did not, neither of them has anything to say that this row has
  #  not said already.
  [ "$f" = "$src" ] && [ -z "$note" ] && srcok=1

  in=$(wc -c < "$f")
  if [ -n "$note" ]; then
    printf '  %-26s %9d  %s\n' "$base" "$in" "$note"
  else
    #  Only files that came back count towards the totals, so the ratio on
    #  the last row is a ratio of the same set of files on both sides of it.
    out=$(wc -c < "$oc")
    counted=$((counted + 1))
    tot_in=$((tot_in + in))
    tot_out=$((tot_out + out))
    tot_enc=$(awk -v a="$tot_enc" -v b="$enc" 'BEGIN{printf "%.3f", a+b}')
    tot_dec=$(awk -v a="$tot_dec" -v b="$dec" 'BEGIN{printf "%.3f", a+b}')
    #  printf(...) with the parentheses, not `printf ...`: inside a bare awk
    #  print list a `>` is a redirection, so the throughput guard below would
    #  quietly write the row to a file called "0" instead of to the terminal.
    awk -v f="$base" -v i="$in" -v o="$out" -v e="$enc" -v d="$dec" 'BEGIN{
      printf("  %-26s %9d %10d %6.2f%% %7.2fs %7.2fs %7.2f\n",
             f, i, o, (i > 0 ? 100.0*o/i : 0), e, d,
             (e > 0 ? i/e/1048576.0 : 0)) }'
  fi

  if [ $keep = 1 ]; then
    cp -f "$oc" "$f.oc" 2>/dev/null || true
    cp -f "$back" "$f.restored" 2>/dev/null || true
  fi
done

printf '  %s\n' '--------------------------------------------------------------------------------'
[ "$counted" = 1 ] && label="1 file" || label="$counted files"
awk -v l="$label" -v i="$tot_in" -v o="$tot_out" -v e="$tot_enc" -v d="$tot_dec" 'BEGIN{
  printf("  %-26s %9d %10d %6.2f%% %7.2fs %7.2fs %7.2f\n",
         l, i, o, (i > 0 ? 100.0*o/i : 0), e, d, (e > 0 ? i/e/1048576.0 : 0)) }'
echo

#  Anything round-trips.  The encoder does not refuse input: what is not
#  Vorbis in it is coded as bytes, so an empty file, a stream cut off in
#  the middle of a page, a .oc fed back to `c`, the scripts in this tree
#  and the compressor's own binary all have to come back byte for byte,
#  with exit 0 on both sides.  The size is printed because it is worth a
#  glance -- a cut-off stream is mostly Vorbis still and should code like
#  it -- but the test is the round trip.
bad_any=0; nany=0

#  Then every way of being told no.  The exit status is the interface here:
#  1 is "this .oc is not one I wrote, or not all of one", 2 is "you typed
#  it wrong", 3 is "the filesystem said no" -- and a run that ends in any of
#  them must not leave an output file, because a half-written .oc that looks
#  like a whole one is how a backup turns out to be nothing.  The removal is
#  unconditional, so a refused run over a file that already existed destroys
#  it -- which is why every output path below is under $tmp.
bad_refusal=0; nrefusal=0

#  Both sections want a good .oc to damage, and take it from the first file
#  under test.  If even that will not code then the round trips above have
#  already failed and there is nothing here left to learn.
if [ $refusals = 1 ] && { [ $srcok = 0 ] ||
     ! "$bin" c "$src" "$tmp/good.oc" >/dev/null 2>&1; }; then
  echo "t.sh: tests after the table skipped -- $src does not code" >&2
  refusals=0
fi

if [ $refusals = 1 ]; then
  : > "$tmp/empty.ogg"

  #  Half the file, up to 400 bytes: a page cut off in the middle, which is
  #  what a stopped download is.  A fixed count would be the whole file for
  #  a small enough input, and a whole file is not a cut-off one.  No real
  #  .ogg is under 400 bytes -- the three headers alone are some kilobytes
  #  -- but the corpus is whatever was named on the command line.
  half() { h=$(( $(wc -c < "$1") / 2 )); [ "$h" -le "$2" ] || h=$2; echo "$h"; }
  head -c "$(half "$src" 400)"          "$src"          > "$tmp/cut.ogg"
  head -c "$(half "$tmp/good.oc" 200)"  "$tmp/good.oc"  > "$tmp/cut.oc"
  cp "$tmp/good.oc" "$tmp/ver.oc"
  printf '\177' | dd of="$tmp/ver.oc" bs=1 seek=5 count=1 conv=notrunc >/dev/null 2>&1
  #  One byte turned, three quarters of the way in: past the headers, so
  #  it is the model's checks and the CRC at the end that have to notice.
  cp "$tmp/good.oc" "$tmp/flip.oc"
  printf '\001' | dd of="$tmp/flip.oc" bs=1 seek="$(( $(wc -c < "$tmp/good.oc") * 3 / 4 ))" \
                      count=1 conv=notrunc >/dev/null 2>&1
  #  A .oc of bytes, not Vorbis, cut in half.  The Vorbis parser's checks
  #  are not there to catch this one: the decoder would turn the missing
  #  half into output for as long as it was let, so it is the cap on
  #  reading past the end that has to stop it, and the CRC that has to
  #  refuse what it made.
  "$bin" c ./t.sh "$tmp/raw.oc" >/dev/null 2>&1
  head -c "$(half "$tmp/raw.oc" 100000)" "$tmp/raw.oc" > "$tmp/rawcut.oc"

  anything() {  # $1 what, $2 the file: code it, decode it, compare
    what=$1; f=$2
    nany=$((nany + 1))
    rm -f "$tmp/any.oc" "$tmp/any.out"
    if "$bin" c "$f" "$tmp/any.oc" >"$tmp/msg" 2>&1 &&
       "$bin" d "$tmp/any.oc" "$tmp/any.out" >>"$tmp/msg" 2>&1 &&
       cmp -s "$f" "$tmp/any.out"; then
      printf '  %-26s %9d -> %8d\n' "$what" "$(wc -c < "$f")" "$(wc -c < "$tmp/any.oc")"
    else
      printf '  %-26s %9d    DID NOT ROUND-TRIP  %s\n' "$what" "$(wc -c < "$f")" \
             "$(sed -n 1p "$tmp/msg" | sed "s|$tmp/||g; s|^[^ ]*: ||" | cut -c1-40)"
      bad_any=$((bad_any + 1))
    fi
  }

  printf '  %s\n' 'anything round-trips'
  anything 'empty input'        "$tmp/empty.ogg"
  anything 'ogg cut off'        "$tmp/cut.ogg"
  anything 'a .oc given to c'   "$tmp/good.oc"
  anything 'a cut-off .oc to c' "$tmp/cut.oc"
  anything 'this script'        ./t.sh
  anything 'the build script'   ./mk.sh
  anything 'the compressor'     "$bin"
  #  A header seen before costs one symbol, not the kilobyte the setup
  #  cost the first time: the same file twice, chained, has to code its
  #  headers for little more than once.  `-v` says what they cost.
  cat "$src" "$src" > "$tmp/twice.ogg"
  anything 'the same file twice'  "$tmp/twice.ogg"
  h1=$("$bin" c -v "$src" "$tmp/h1.oc" 2>&1 | awk '/^  headers/ {print $2}')
  h2=$("$bin" c -v "$tmp/twice.ogg" "$tmp/h2.oc" 2>&1 | awk '/^  headers/ {print $2}')
  if [ -z "$h1" ] || [ -z "$h2" ]; then
    printf '  %-26s %s\n' 'headers seen twice' "not checked: $src has no Vorbis headers"
  elif [ "$h2" -lt $(( h1 * 5 / 4 + 64 )) ]; then
    printf '  %-26s %s\n' 'headers seen twice' "$h1 bytes once, $h2 for both"
  else
    printf '  %-26s %s\n' 'headers seen twice' "FAILED  $h1 bytes once, $h2 for both"
    bad_any=$((bad_any + 1))
  fi
  echo

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
                sed "s|$tmp/||g; s|^[^ ]*: ||" |
                cut -c1-48)"
    fi
  }

  printf '  %s\n' 'refusals'
  refuse 'ogg given to d'      1 d "$src"           "$tmp/no.out"
  refuse 'empty .oc'           1 d "$tmp/empty.ogg" "$tmp/no.out"
  refuse 'truncated .oc'       1 d "$tmp/cut.oc"    "$tmp/no.out"
  refuse 'truncated .oc of bytes' 1 d "$tmp/rawcut.oc" "$tmp/no.out"
  refuse 'a byte turned in a .oc' 1 d "$tmp/flip.oc" "$tmp/no.out"
  refuse 'wrong version byte'  1 d "$tmp/ver.oc"    "$tmp/no.out"
  refuse 'input not there'     3 c "$tmp/nothing"   "$tmp/no.out"
  refuse 'output over input'   2 c "$src"           "$src"
  refuse 'no arguments'        2
  refuse 'unknown mode'        2 z "$src"           "$tmp/no.out"
  refuse 'unknown option'      2 c -Z "$src"        "$tmp/no.out"
  echo
fi

#  A lone `-` is a path meaning stdin on the way in and stdout on the way
#  out, which is not a thing one can tell from the usage text, and the only
#  part of the interface with no file behind it.
if [ $srcok = 1 ]; then
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

#  oggdet, the carver, is the compressor's other front end.  `oggdet c`
#  finds every Ogg stream in a file that may hold anything else too -- an
#  archive, a disk image, a download that stopped -- writes each one out,
#  and keeps the bytes between them and the layout in a .meta, from which
#  `oggdet d` puts the file back.  With -c all of it goes into one file:
#  every stream a segment that is the .oc `oggcomp c` would make of it --
#  the same model, in the same process, reset between segments -- and the
#  bytes between them coded too, the layout with them.  So the container
#  has to come back byte for byte, with and without -c, and a stream's
#  segment has to be the .oc of that stream on its own -- which is also
#  what makes it the compressor's to decode.  The container built here
#  holds the first file under test twice, once with every LF turned into
#  CR LF -- a stream that went through a text-mode transfer, whose LFs the
#  carver puts back -- the corpus file with a cover image in it when the
#  corpus is there, and this tree's scripts as the bytes between them.
bad_det=0; ndet=0
det=$(dirname -- "$bin")/oggdet
if [ $refusals = 1 ] && [ $srcok = 1 ] && [ ! -x "$det" ]; then
  echo "t.sh: oggdet tests skipped -- $det is not there (./mk.sh builds it beside oggcomp)" >&2
elif [ $refusals = 1 ] && [ $srcok = 1 ]; then
  dd=$tmp/det; mkdir -p "$dd"
  art=testfiles/art-8k.ogg; [ -f "$art" ] || art=
  perl -pe 's/\n/\r\n/g' < "$src" > "$dd/crlf.ogg"
  { head -c 3000 ./mk.sh; cat "$src"; head -c 700 ./t.sh; cat "$src"
    head -c 100 ./mk.sh; cat "$dd/crlf.ogg"; head -c 2000 ./t.sh
    [ -z "$art" ] || cat "$art"; head -c 500 ./mk.sh; } > "$dd/box"

  detcheck() {  # $1 what, $2 1 if it passed, $3 what to say about it
    ndet=$((ndet + 1))
    if [ "$2" = 1 ]; then printf '  %-26s %s\n' "$1" "$3"
    else printf '  %-26s FAILED  %s\n' "$1" "$3"; bad_det=$((bad_det + 1)); fi
  }
  #  The carver's last line on stderr is its summary, minus the path.
  detsaid() { sed -n '$p' "$tmp/msg" | sed "s|$tmp/||g; s|^[^ ]*: ||" | cut -c1-56; }
  #  One stream's segment cut out of the file -c wrote, to stdout: eight
  #  bytes of magic, one of flags and one naming the model, then segments,
  #  each a varint of length times four plus kind -- 0 metainfo, 1 stream,
  #  2 image -- and the bytes.  perl, which the build needs anyway.
  arseg() {  # $1 the file, $2 which stream, counted from 0
    perl -e 'binmode STDIN; binmode STDOUT; local $/; my $d = <STDIN>;
             exit 1 unless substr($d, 0, 8) eq "OGGDETc2";
             my ($p, $i) = (10, 0);
             while ($p < length $d) {
               my ($v, $s, $b) = (0, 0, 0);
               do { $b = ord substr($d, $p++, 1); $v |= ($b & 127) << $s; $s += 7 }
                 while ($b & 128);
               my ($n, $k) = ($v >> 2, $v & 3);
               if ($k == 1) { if ($i == $ARGV[0]) { print substr($d, $p, $n); exit 0 } $i++ }
               $p += $n;
             }
             exit 1' "$2" < "$1"
  }
  #  Carve, restore, compare: $1 what to call it, $2 what the carve writes,
  #  the rest its options.  Leaves the carve's summary in $tmp/msg.
  dettrip() {
    what=$1; out=$2; shift 2
    dok=0
    if "$det" c "$@" "$dd/box" "$out" >"$tmp/msg" 2>&1 &&
       "$det" d "$out" "$out.back" >/dev/null 2>&1 &&
       cmp -s "$dd/box" "$out.back"; then dok=1; fi
    detcheck "$what" $dok "$(detsaid)"
  }

  printf '  %s\n' 'oggdet'
  #  Plain, with cover art left in place and no size floor: the first
  #  stream it writes is then the first file under test, whole, if that
  #  file is one stream and nothing else -- which the next check needs.
  dettrip 'carve and restore' "$dd/p" -A -s 0
  #  -c, into one file.  -D: the copies are coded too, so the second and
  #  third segment are the first file again -- once as it is, once with
  #  its LFs put back -- and a model reset between segments shows as
  #  their being the first.
  dettrip 'carve -c and restore' "$dd/c" -c -A -s 0 -D
  if [ -f "$dd/p00000000.ogg" ] && cmp -s "$dd/p00000000.ogg" "$src"; then
    dok=0
    if arseg "$dd/c" 0 > "$dd/c0.oc" && cmp -s "$dd/c0.oc" "$tmp/good.oc" &&
       "$bin" d "$dd/c0.oc" "$dd/c0.ogg" >/dev/null 2>&1 &&
       cmp -s "$dd/c0.ogg" "$src"; then dok=1; fi
    detcheck "segment 0 is oggcomp's" $dok "$(wc -c < "$tmp/good.oc") bytes either way, and oggcomp d reads it"
    dok=0
    if arseg "$dd/c" 1 > "$dd/c1.oc" && arseg "$dd/c" 2 > "$dd/c2.oc" &&
       cmp -s "$dd/c1.oc" "$tmp/good.oc" && cmp -s "$dd/c2.oc" "$tmp/good.oc"; then dok=1; fi
    detcheck "so are 1 and 2" $dok "coded after a reset, and after a CRLF repair"
  else
    printf '  %-26s %s\n' "segment 0 is oggcomp's" "not checked: $src is not one whole stream"
  fi

  #  -S: the model kept from one segment to the next.  The container still
  #  comes back; the second copy of the first file costs less than it did
  #  fresh, since the model has seen it; and its segment is not the
  #  compressor's -- it decodes only after the one before it.
  dettrip 'carve -S and restore' "$dd/s" -S -A -s 0 -D
  if [ -s "$dd/c1.oc" ] && arseg "$dd/s" 1 > "$dd/s1.oc"; then
    fresh=$(wc -c < "$dd/c1.oc"); solid=$(wc -c < "$dd/s1.oc")
    dok=0; [ "$solid" -lt "$fresh" ] && dok=1
    detcheck 'the 2nd copy costs less' $dok "$fresh bytes fresh, $solid solid"
    rm -f "$tmp/no.out"
    set +e
    "$bin" d "$dd/s1.oc" "$tmp/no.out" >"$tmp/msg" 2>&1
    got=$?
    set -e
    dok=0; [ "$got" = 1 ] && [ ! -e "$tmp/no.out" ] && dok=1
    detcheck "and is not oggcomp's" $dok "oggcomp d exits $got  $(detsaid | cut -c1-30)"
  fi

  #  Art on, at the default floor: the picture comes out of the stream
  #  into a segment of its own, the stream is rewritten without it, and
  #  the original still has to come back.
  dettrip 'carve -c, art taken out' "$dd/a" -c
  if [ -n "$art" ]; then
    dok=0; case "$(detsaid)" in *image*) dok=1;; esac
    detcheck 'the cover image' $dok "$(detsaid | cut -c1-44)"
  fi

  #  -m: the metainfo in a file of its own, which the file then needs.
  dok=0
  if "$det" c -c -A -s 0 -m "$dd/m.meta" "$dd/box" "$dd/m" >"$tmp/msg" 2>&1 &&
     "$det" d -m "$dd/m.meta" "$dd/m" "$dd/m.back" >/dev/null 2>&1 &&
     cmp -s "$dd/box" "$dd/m.back"; then dok=1; fi
  detcheck 'carve -c -m and restore' $dok "$(detsaid)"

  #  -t looks and writes nothing.
  dok=1
  "$det" c -c -t "$dd/box" "$dd/t" >"$tmp/msg" 2>&1 || dok=0
  for q in "$dd"/t*; do [ -e "$q" ] && dok=0; done
  detcheck '-t writes nothing' $dok "$(detsaid)"

  #  And the refusals, with the output file gone afterwards.
  detrefuse() {  # $1 what, $2 the exit status wanted, rest: arguments to oggdet
    what=$1; want=$2; shift 2
    rm -f "$tmp/no.out"
    set +e
    "$det" "$@" "$tmp/no.out" >"$tmp/msg" 2>&1
    got=$?
    set -e
    if [ "$got" = "$want" ] && [ ! -e "$tmp/no.out" ]; then detcheck "$what" 1 "exit $got  $(detsaid)"
    elif [ -e "$tmp/no.out" ]; then detcheck "$what" 0 "exit $got, but left an output file behind"
    else detcheck "$what" 0 "exit $got, wanted $want"; fi
  }
  cp "$src" "$dd/bad.meta"
  head -c 300 "$dd/p.meta" > "$dd/cut.meta"
  head -c "$(( $(wc -c < "$dd/c") / 2 ))" "$dd/c" > "$dd/short"
  cp "$dd/c" "$dd/flip"
  printf '\001' | dd of="$dd/flip" bs=1 seek="$(( $(wc -c < "$dd/c") * 3 / 4 ))" \
                      count=1 conv=notrunc >/dev/null 2>&1
  detrefuse 'a .meta that is not one'    1 d "$dd/bad"
  detrefuse 'a .meta cut short'          1 d "$dd/cut"
  detrefuse 'a prefix with no .meta'     3 d "$src"
  detrefuse 'the file cut short'         1 d "$dd/short"
  detrefuse 'a byte turned in the file'  1 d "$dd/flip"
  detrefuse 'made with -m, read without' 1 d "$dd/m"
  #  Named over its own input: refused, and the input as it was afterwards.
  cp "$dd/c" "$dd/c.was"
  set +e
  "$det" d "$dd/c" "$dd/c" >"$tmp/msg" 2>&1
  got=$?
  set -e
  if [ "$got" = 2 ] && cmp -s "$dd/c" "$dd/c.was"; then detcheck 'output named as the input' 1 "exit 2  $(detsaid)"
  else detcheck 'output named as the input' 0 "exit $got, input $(wc -c < "$dd/c") bytes"; fi
  echo
fi

#  The model as a library.  oc_api.h's functions come linked into the
#  program as model 0 and exported by oggcompN.so as model N, and the
#  programs drive either through the same table of pointers (oc_load.inc),
#  so -1 with a library built from the tree's own IDX has to write what -0
#  writes, but for the header byte that names the model; the stream names
#  it, so `d` finds the library on its own and refuses -0; and a library
#  that is not there is a refusal with nothing written.  The library is
#  the shipping form of the model, so on a tuning ./oggcomp this is also
#  the two forms against each other, which `./mk.sh check` does at length.
bad_lib=0; nlib=0
lib=$(dirname -- "$bin")/oggcomp1.so
if [ $refusals = 1 ] && [ $srcok = 1 ] && [ ! -f "$lib" ]; then
  echo "t.sh: library tests skipped -- $lib is not there (./mk.sh dll 1 builds it)" >&2
elif [ $refusals = 1 ] && [ $srcok = 1 ]; then
  libcheck() {  # $1 what, $2 1 if it passed, $3 what to say
    nlib=$((nlib + 1))
    if [ "$2" = 1 ]; then printf '  %-26s %s\n' "$1" "$3"
    else printf '  %-26s FAILED  %s\n' "$1" "$3"; bad_lib=$((bad_lib + 1)); fi
  }
  printf '  %s\n' 'the model as a library'
  lok=0
  if "$bin" c -1 "$src" "$tmp/lib1.oc" >"$tmp/msg" 2>&1 &&
     tail -c +8 "$tmp/good.oc" > "$tmp/lib0.body" && tail -c +8 "$tmp/lib1.oc" > "$tmp/lib1.body" &&
     cmp -s "$tmp/lib0.body" "$tmp/lib1.body" &&
     [ "$(head -c 7 "$tmp/lib1.oc" | tail -c 1 | od -An -tu1 | tr -d ' ')" = 1 ]; then lok=1; fi
  libcheck 'c -1 codes as c -0' $lok "$(wc -c < "$tmp/lib1.oc") bytes, the model byte 1"
  lok=0
  if "$bin" d "$tmp/lib1.oc" "$tmp/lib1.ogg" >"$tmp/msg" 2>&1 && cmp -s "$src" "$tmp/lib1.ogg"; then lok=1; fi
  libcheck 'd finds the model itself' $lok "from the header, no option given"
  for q in 'd -0 on a model-1 stream:1:d -0 LIB1:' 'a library not there:3:c -9 SRC:'; do
    what=${q%%:*}; rest=${q#*:}; want=${rest%%:*}; args=${rest#*:}; args=${args%:}
    rm -f "$tmp/no.out"
    set +e
    case "$args" in
      *LIB1*) "$bin" ${args%LIB1} "$tmp/lib1.oc" "$tmp/no.out" >"$tmp/msg" 2>&1;;
      *)      "$bin" ${args%SRC} "$src" "$tmp/no.out" >"$tmp/msg" 2>&1;;
    esac
    got=$?
    set -e
    if [ "$got" = "$want" ] && [ ! -e "$tmp/no.out" ]; then libcheck "$what" 1 "exit $got  $(sed -n 1p "$tmp/msg" | sed "s|$tmp/||g; s|^[^ ]*: ||" | cut -c1-44)"
    elif [ -e "$tmp/no.out" ]; then libcheck "$what" 0 "exit $got, but left an output file behind"
    else libcheck "$what" 0 "exit $got, wanted $want"; fi
  done
  det=$(dirname -- "$bin")/oggdet
  if [ -x "$det" ] && [ -f "$tmp/det/box" ]; then
    lok=0
    if "$det" c -c -1 -A -s 0 -D "$tmp/det/box" "$tmp/det/l1" >"$tmp/msg" 2>&1 &&
       "$det" d "$tmp/det/l1" "$tmp/det/l1.back" >/dev/null 2>&1 &&
       cmp -s "$tmp/det/box" "$tmp/det/l1.back"; then lok=1; fi
    libcheck 'oggdet -c -1 and back' $lok "$(sed -n '$p' "$tmp/msg" | sed "s|$tmp/||g; s|^[^ ]*: ||" | cut -c1-44)"
  fi
  echo
fi

printf 't.sh: %d/%d round-tripped byte for byte' "$ok" "$n"
[ $twice = 1 ] && printf ', %d/%d coded identically twice' "$same" "$nsame"
echo
[ $refusals = 1 ] &&
  printf 't.sh: %d/%d other inputs round-tripped, %d/%d refusals gave the exit status they should\n' \
         "$((nany - bad_any))" "$nany" "$((nrefusal - bad_refusal))" "$nrefusal"
[ $ndet -gt 0 ] &&
  printf 't.sh: %d/%d oggdet checks passed\n' "$((ndet - bad_det))" "$ndet"
[ $nlib -gt 0 ] &&
  printf 't.sh: %d/%d library checks passed\n' "$((nlib - bad_lib))" "$nlib"

if [ $fail = 0 ] && [ $bad_any = 0 ] && [ $bad_refusal = 0 ] && [ $bad_det = 0 ] && [ $bad_lib = 0 ] && [ $n -gt 0 ]; then
  echo "t.sh: PASS"
else
  echo "t.sh: FAIL" >&2
  exit 1
fi
