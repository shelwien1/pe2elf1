#!/bin/sh
#  Regenerate testfiles/*.ogg -- the corpus ./t.sh round-trips and ./mk.sh
#  check codes under both builds.
#
#      ./testfiles/gen.sh          encode whatever is missing
#      ./testfiles/gen.sh -f       encode all of it again
#
#  The corpus is checked in, so nothing normally runs this: a fresh clone can
#  test the compressor with no encoder installed, which is the point.  It is
#  here because a corpus one cannot regenerate is a corpus one cannot explain,
#  and because the next parser bug will want a file that is not in it yet.
#
#  Needs oggenc (vorbis-tools) and python3; neither is needed to build or to
#  test.  Every stream is encoded with an explicit --serial, so the bytes are
#  reproducible: without one oggenc picks the serial number at random, and two
#  runs would then produce two different corpora of identical audio -- which
#  would make `git diff` useless for reviewing a change to this file.  The
#  reproducibility is per libvorbis: another version may pack the same audio
#  differently, and what that produces is a new corpus, not a broken one.
#
#  What each file is for is in testfiles/README.md, and in the comments here.
#  To add one: synthesize it in the python below, add an encode line, run
#  this, and check in what comes out.

set -e

here=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
cd "$here"

force=0
[ "${1:-}" = "-f" ] && force=1

command -v oggenc >/dev/null ||
  { echo "gen.sh: needs oggenc -- apt-get install vorbis-tools" >&2; exit 2; }
command -v python3 >/dev/null ||
  { echo "gen.sh: needs python3" >&2; exit 2; }

want() {  # is $1.ogg still to be made?
  [ $force = 1 ] || [ ! -f "$1.ogg" ] && return 0
  echo "  $1.ogg  (kept)"
  return 1
}

#  $1 name, $2 serial, rest oggenc flags ending in the source wav.  The serial
#  is fixed per file and never reused, so the chained stream built at the end
#  carries two distinct serials, which is what a real one looks like.
enc() {
  name=$1; serial=$2; shift 2
  oggenc -Q --serial "$serial" "$@" -o "$name.ogg"
  echo "  $name.ogg  $(wc -c < "$name.ogg") bytes"
}

#  Nothing to synthesize if every file is already here.  Worth the four
#  lines: the audio below takes a few seconds to write and a run that has
#  nothing to do should say so and stop.
need=0
for n in sine-stereo-q5 noise-stereo-q3 mono-22k-q4 silence-stereo-q4 \
         tiny-8k-q0 sweep-mono-qm1 chirp-stereo-q10 multi6-48k-q4 \
         music-stereo-q5 music-managed-b96 bigcomment-8k chained; do
  [ -f "$n.ogg" ] || need=1
done
[ -d refused ] || need=1

if [ $force = 1 ] || [ $need = 1 ]; then
#  One WAV per stream, all of it synthesized -- no sample of anyone's music is
#  checked into a compressor's test corpus -- and seeded, so the same script
#  gives the same waveform on any machine.  The big comment is written here
#  too: it has to be 64 kB to be worth having (see below) and a shell is the
#  wrong place to build 64 kB of anything.
python3 - <<'PY'
import base64, math, random, struct, wave

SR = 44100

def write(path, ch, sr, frames, fn):
    w = wave.open(path, 'wb')
    w.setnchannels(ch); w.setsampwidth(2); w.setframerate(sr)
    buf = bytearray()
    for i in range(frames):
        for c in range(ch):
            v = fn(i, c, sr)
            if v < -32768: v = -32768
            if v > 32767:  v = 32767
            buf += struct.pack('<h', int(v))
    w.writeframes(bytes(buf)); w.close()

r = random.Random(20240612)

#  Two steady tones, one per channel.  Nearly all floor and almost no
#  residue: the shape a vorbis stream takes when the encoder has an easy
#  time, and the file where the floor model earns or loses its keep.
write('sine-stereo.wav', 2, SR, SR * 3,
      lambda i, c, sr: 12000 * math.sin(2 * math.pi * (440 + 110 * c) * i / sr))

#  White noise: the other end.  The residue books carry everything and the
#  encoder's output is already close to incompressible, so this is the file
#  that says whether the model is paying for itself or just spending time.
write('noise-stereo.wav', 2, SR, SR * 2,
      lambda i, c, sr: r.randint(-9000, 9000))

#  Mono, and at a rate that is not 44100, so neither the channel count nor
#  the sample rate in the identity header is the one every other file has.
write('mono-22k.wav', 1, 22050, 22050 * 2,
      lambda i, c, sr: 9000 * math.sin(2 * math.pi * 330 * i / sr)
                     + 2500 * math.sin(2 * math.pi * 1319 * i / sr))

#  Digital silence.  Vorbis codes it as a floor of nothing and empty residue
#  partitions, and a packet that decodes to no bits at all is exactly the
#  degenerate case a range coder gets wrong.
write('silence-stereo.wav', 2, SR, SR * 2, lambda i, c, sr: 0)

#  Short enough that the whole stream is a few pages: the three headers are
#  most of the file and the audio is a handful of packets.  Also the source
#  of the last page of a stream being a short one, which is where the
#  end-of-stream flag and a granulepos that is not a multiple of anything
#  have to survive the round trip.
write('tiny-8k.wav', 1, 8000, 900,
      lambda i, c, sr: 6000 * math.sin(2 * math.pi * 400 * i / sr))

#  A sweep crosses every band the floor curve has, so consecutive packets
#  never share a floor -- the opposite of the steady tones above, and the
#  thing the floor model's context is actually predicting from.
write('sweep-mono.wav', 1, SR, SR * 3,
      lambda i, c, sr: 11000 * math.sin(2 * math.pi * (80 * math.exp(4.2 * i / (sr * 3.0))) * i / sr))

#  Fast chirps over a noise floor, to be encoded at -q 10 where the encoder
#  reaches for the long blocksize and the widest books it has.
def chirp(i, c, sr):
    t = i / float(sr)
    f = 200 + 3000 * abs(math.sin(2 * math.pi * 3 * t + c))
    return 10000 * math.sin(2 * math.pi * f * t) + 700 * r.uniform(-1, 1)
write('chirp-stereo.wav', 2, SR, int(SR * 1.5), chirp)

#  Six channels: vorbis maps 6 as 5.1, and the channel count drives both the
#  coupling declared in the setup header and how many residue vectors each
#  packet carries.
write('multi6-48k.wav', 6, 48000, 24000,
      lambda i, c, sr: 8000 * math.sin(2 * math.pi * (200 + 80 * c) * i / sr))

#  The nearest thing here to music: six harmonically related partials, a
#  percussive envelope twice a second, and hiss.  This is the file whose
#  compressed size is worth watching between revisions of the model, and it
#  is encoded twice -- once pure VBR, once bitrate-managed, which makes the
#  encoder pad and truncate packets and so changes what the model sees.
def music(i, c, sr):
    t = i / float(sr)
    env = math.exp(-(i % (sr // 2)) / (sr * 0.08))
    v = 0.0
    for k, f in enumerate((220.0, 277.18, 329.63, 440.0, 554.37, 659.25)):
        v += 3000.0 / (k + 1) * math.sin(2 * math.pi * f * (1 + 0.001 * c) * t + 0.3 * k)
    return v * (0.4 + 0.6 * env) + 1200 * env * r.uniform(-1, 1) + 300 * r.uniform(-1, 1)
write('music-stereo.wav', 2, SR, SR * 8, music)

#  A comment value of 72 kB.  An Ogg page holds at most 255 segments of 255
#  bytes, so a packet above 65025 bytes CANNOT fit in one page: the encoder
#  has to split it and set the continuation flag on the page that carries
#  the rest.  Nothing else in this corpus reaches that path, and real files
#  take it all the time -- an embedded cover image is a base64 comment of
#  exactly this shape, which is what this pretends to be.  Random bytes, so
#  the parser cannot be right by accident on a run of one value.
blob = base64.b64encode(bytes(r.randrange(256) for _ in range(54000))).decode()
open('bigcomment.txt', 'w').write('METADATA_BLOCK_PICTURE=' + blob)
PY
fi

want sine-stereo-q5     && enc sine-stereo-q5     1001 -q 5    sine-stereo.wav
want noise-stereo-q3    && enc noise-stereo-q3    1002 -q 3    noise-stereo.wav
want mono-22k-q4        && enc mono-22k-q4        1003 -q 4    mono-22k.wav
want silence-stereo-q4  && enc silence-stereo-q4  1004 -q 4    silence-stereo.wav
want tiny-8k-q0         && enc tiny-8k-q0         1005 -q 0    tiny-8k.wav
want sweep-mono-qm1     && enc sweep-mono-qm1     1006 -q -1   sweep-mono.wav
want chirp-stereo-q10   && enc chirp-stereo-q10   1007 -q 10   chirp-stereo.wav
want multi6-48k-q4      && enc multi6-48k-q4      1008 -q 4    multi6-48k.wav
want music-stereo-q5    && enc music-stereo-q5    1009 -q 5    music-stereo.wav

#  --managed makes the encoder hit an average bitrate rather than a quality,
#  which it does by padding and truncating packets: same audio as the file
#  above, a different distribution of packet lengths.
want music-managed-b96  && enc music-managed-b96  1010 -b 96 --managed music-stereo.wav

#  The oversized comment goes on the smallest audio, so the file is mostly
#  the header packet whose page-splitting it is here to exercise.
want bigcomment-8k      && enc bigcomment-8k      1011 -q 0 \
                               -c "$(cat bigcomment.txt)" tiny-8k.wav

#  Two logical bitstreams in one file, which is what a `cat` of two .oggs is
#  and what a stream ripped from an internet radio looks like: the second
#  stream's headers arrive in the middle of the file, with a different
#  serial, sample rate and channel count from the first.  Built from the
#  encoded files rather than encoded on its own, so its parts are files that
#  are themselves in the corpus and it changes only when they do.
if want chained; then
  cat mono-22k-q4.ogg tiny-8k-q0.ogg > chained.ogg
  echo "  chained.ogg  $(wc -c < chained.ogg) bytes  (mono-22k-q4 + tiny-8k-q0)"
fi

#  testfiles/refused/ -- inputs oggcomp must decline, one per way of being
#  wrong.  Half of what a lossless compressor promises is that it will not
#  pretend: a file it cannot give back exactly has to be refused, loudly and
#  with nothing written, rather than coded into a stream that decodes to
#  something else.  t.sh checks that each of these still exits 1.
#
#  They are derived from tiny-8k-q0.ogg, so they are small and so a diff
#  against them means something.  Anything with a rewritten page needs its
#  CRC recomputed, or the file would be refused for the CRC and the case it
#  was built for would never be reached -- which is the mistake worth being
#  careful about here.
if [ $force = 1 ] || [ ! -d refused ]; then
  mkdir -p refused
  python3 - <<'REFUSED'
import struct

src = open('tiny-8k-q0.ogg', 'rb').read()

def crc32_ogg(data):
    #  Ogg's CRC-32: polynomial 0x04c11db7, no reflection, no initial or
    #  final inversion.  Not the zlib one, and nothing in the standard
    #  library computes it.
    poly, tbl = 0x04c11db7, []
    for i in range(256):
        r = i << 24
        for _ in range(8):
            r = ((r << 1) ^ poly) & 0xffffffff if r & 0x80000000 else (r << 1) & 0xffffffff
        tbl.append(r)
    c = 0
    for b in data:
        c = ((c << 8) & 0xffffffff) ^ tbl[((c >> 24) & 0xff) ^ b]
    return c

def pages(d):
    off = 0
    while off < len(d) and d[off:off + 4] == b'OggS':
        nseg = d[off + 26]
        end = off + 27 + nseg + sum(d[off + 27:off + 27 + nseg])
        yield off, end
        off = end

def repage(page):
    p = bytearray(page)
    p[22:26] = b'\0\0\0\0'
    p[22:26] = struct.pack('<I', crc32_ogg(bytes(p)))
    return bytes(p)

last = list(pages(src))[-1]

#  One flipped byte in the last page's payload, with the stored CRC left as
#  it was: "the page at N has a bad CRC".
d = bytearray(src); d[last[1] - 1] ^= 0xff
open('refused/badcrc.ogg', 'wb').write(bytes(d))

#  The end-of-stream flag cleared on the last page, and the page re-CRCed so
#  that it is the missing flag that gets noticed: "final bitstream has no
#  end-of-stream page".  A stream that merely stops is a truncated download,
#  and coding one would silently invent an ending for it.
d = bytearray(src)
p = bytearray(src[last[0]:last[1]]); p[5] &= 0xfb
d[last[0]:last[1]] = repage(bytes(p))
open('refused/no-eos.ogg', 'wb').write(bytes(d))

#  An ID3v2 tag in front of the first page.  Players skip it; something that
#  has to give the file back byte for byte cannot: "no Ogg page at 0".
open('refused/id3-prefix.ogg', 'wb').write(
    b'ID3\x04\x00\x00\x00\x00\x00\x20' + b'\0' * 32 + src)

#  Junk after the end-of-stream page -- the shape of a file concatenated
#  with something that is not Ogg: "no Ogg page at <size of the real file>".
open('refused/trailing-junk.ogg', 'wb').write(src + b'RIFFjunkjunkjunk')

#  Ogg Opus: a well-formed Ogg page whose first packet is an OpusHead.  The
#  container is fine and the codec is not, which is a different refusal from
#  every one above and the one a user is most likely to meet.
head = (b'OpusHead' + bytes([1, 2]) + struct.pack('<H', 312)
        + struct.pack('<I', 48000) + struct.pack('<h', 0) + bytes([0]))
open('refused/opus.ogg', 'wb').write(repage(
    b'OggS' + bytes([0, 0x02]) + struct.pack('<q', 0)
    + struct.pack('<I', 0x0badface) + struct.pack('<I', 0)
    + struct.pack('<I', 0) + bytes([1, len(head)]) + head))
REFUSED
  for r in refused/*.ogg; do echo "  $r  $(wc -c < "$r") bytes"; done
else
  echo "  refused/  (kept)"
fi

rm -f ./*.wav bigcomment.txt
echo "gen.sh: $(ls -1 ./*.ogg | wc -l) files, $(cat ./*.ogg | wc -c) bytes," \
     "$(ls -1 refused/*.ogg | wc -l) more to be refused"
