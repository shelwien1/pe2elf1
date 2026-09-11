#!/bin/sh
#  Regenerate testfiles/* -- the corpus ./t.sh round-trips and ./mk.sh
#  check codes under both builds: Vorbis streams from oggenc, and the files
#  derived from them that are not quite Vorbis, or not Ogg at all.
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
         music-stereo-q5 music-managed-b96 bigcomment-8k chained \
         uncoupled-stereo-q4 silence-8k-long-qm1 zerolen-8k-q0 tags-many-8k \
         skeleton-8k minbitrate-pad art-8k; do
  [ -f "$n.ogg" ] || need=1
done

if [ $force = 1 ] || [ $need = 1 ]; then
#  One WAV per stream, all of it synthesized -- no sample of anyone's music is
#  checked into a compressor's test corpus -- and seeded, so the same script
#  gives the same waveform on any machine.  The big comment is written here
#  too: it has to be 64 kB to be worth having (see below) and a shell is the
#  wrong place to build 64 kB of anything.
python3 - <<'PY'
import base64, math, random, struct, wave, zlib

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

#  Stereo tones to encode with coupling turned off.  libvorbis couples only
#  2- and 6-channel streams, and an uncoupled pair is the one shape that
#  gives residue type 1 with more than one vector -- a different branch from
#  every other stereo file here, and one no quality setting reaches.
write('tone2.wav', 2, SR, SR,
      lambda i, c, sr: 11000 * math.sin(2 * math.pi * (300 + 90 * c) * i / sr))

#  Ninety seconds of 8 kHz silence at -q -1.  Each packet is one byte, so
#  the encoder fills pages to their 255-segment ceiling: 255 packets ending
#  in one page, which is as many as an Ogg page can hold and which nothing
#  else in this corpus comes near.  It costs 8 kB.
write('sil8k.wav', 1, 8000, 8000 * 90, lambda i, c, sr: 0)

#  No samples at all.  oggenc still writes the three headers and one audio
#  packet, which is the smallest legal Vorbis stream there is.
write('zero8k.wav', 1, 8000, 0, lambda i, c, sr: 0)

#  A second of 44100 stereo silence, to be encoded with a minimum bitrate
#  it cannot possibly fill.  libvorbis then pads the audio packets with zero
#  bits -- bits the Vorbis spec does not account for, which the packet
#  coder has to notice and put back.
write('sil44.wav', 2, SR, SR, lambda i, c, sr: 0)

#  Two comment values of 66 kB each.  An Ogg page holds at most 255 segments
#  of 255 bytes, so a packet above 65025 bytes CANNOT fit in one page and the
#  encoder splits it, setting the continuation flag on the pages that carry
#  the rest.  Above 130050 it needs three, and the middle one is both a
#  continuation and itself continued -- a page shape two pages never make,
#  and the reason this is 133 kB rather than the 72 kB that would do for a
#  simple split.  Those pages also carry granulepos -1, which is the only
#  place in the corpus the granulepos coder sees one.  Real files take this
#  path constantly: an embedded cover image is a base64 comment of exactly
#  this shape, which is what these pretend to be.  Random bytes, so the
#  parser cannot be right by accident on a run of one value.  Two values
#  rather than one because a single argument cannot exceed 128 kB.
for k in range(2):
    blob = base64.b64encode(bytes(r.randrange(256) for _ in range(50000))).decode()
    open('bigcomment%d.txt' % k, 'w').write('METADATA_BLOCK_PICTURE%d=%s' % (k, blob))

#  A cover image the way a tagger stores one: a METADATA_BLOCK_PICTURE
#  comment holding a FLAC PICTURE block -- type, MIME, description, the
#  dimensions, the image -- as base64.  The image is a 24 by 24 PNG of
#  random pixels, stored rather than deflated so the bytes do not depend
#  on the zlib version, and just over the kilobyte below which oggdet
#  leaves a picture in the stream.  For the compressor it is a comment
#  header of the shape real files have; for oggdet it is the file its
#  art extraction is tested on.  Its own generator, so that adding it
#  changed no file above.
def chunk(kind, data):
    return (struct.pack('>I', len(data)) + kind + data
            + struct.pack('>I', zlib.crc32(kind + data) & 0xffffffff))
ra = random.Random(20240614)
rows = b''.join(b'\x00' + bytes(ra.randrange(256) for _ in range(24 * 3))
                for _ in range(24))
png = (b'\x89PNG\r\n\x1a\n'
       + chunk(b'IHDR', struct.pack('>IIBBBBB', 24, 24, 8, 2, 0, 0, 0))
       + chunk(b'IDAT', zlib.compress(rows, 0))
       + chunk(b'IEND', b''))
mime, desc = b'image/png', b'front cover'
pic = (struct.pack('>II', 3, len(mime)) + mime + struct.pack('>I', len(desc)) + desc
       + struct.pack('>IIIII', 24, 24, 24, 0, len(png)) + png)
open('art.txt', 'w').write('METADATA_BLOCK_PICTURE=' + base64.b64encode(pic).decode())
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

#  The oversized comments go on the smallest audio, so the file is mostly
#  the header packet whose page-splitting it is here to exercise.
want bigcomment-8k      && enc bigcomment-8k      1011 -q 0 \
                               -c "$(cat bigcomment0.txt)" \
                               -c "$(cat bigcomment1.txt)" tiny-8k.wav

#  Coupling off: the uncoupled multi-vector residue path.  oggenc says on
#  stderr that it is setting the option; that line is not an error, and
#  silencing stderr here would silence the ones that are.
want uncoupled-stereo-q4 && enc uncoupled-stereo-q4 1012 -q 4 \
                               --advanced-encode-option disable_coupling=1 tone2.wav

#  Pages filled to the 255-packet ceiling.
want silence-8k-long-qm1 && enc silence-8k-long-qm1 1013 -q -1 sil8k.wav

#  The smallest legal Vorbis stream: three headers and one audio packet.
want zerolen-8k-q0      && enc zerolen-8k-q0      1014 -q 0 zero8k.wav

#  Every byte of the comment header goes through the model one at a time, so
#  what is in the tags matters as much as how big they are.  This is the
#  ordinary case the two big-comment files are not: a dozen short tags, a
#  date, a track number, non-ASCII, and one value with spaces in it.
want tags-many-8k && enc tags-many-8k 1015 -q 0 \
      -t "T\303\256tle \303\274nicode" -a "Artist \303\221ame" -l "\303\201lbum" \
      -G Genre -d 2024-06-12 -N 7 \
      -c "REPLAYGAIN_TRACK_GAIN=-3.21 dB" -c "A=1" -c "B=2" -c "C=3" \
      -c "DESCRIPTION=a longer value, with spaces and punctuation in it" \
      tiny-8k.wav

#  Cover art, as a tagger embeds it: one METADATA_BLOCK_PICTURE comment of
#  2.5 kB of base64, on the smallest audio.  oggdet takes the picture out
#  of the comment header and has to put it back to the byte.
want art-8k && enc art-8k 1016 -q 0 -c "$(cat art.txt)" tiny-8k.wav

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

#  Two more from oggenc that are not plain Vorbis-in-Ogg, which is what
#  makes them worth having: they are not damaged files, they are files a
#  working encoder writes, and each one once made oggcomp refuse.
#
#  -k multiplexes an Ogg Skeleton bitstream in beside the Vorbis one: a sound
#  container holding something besides one Vorbis stream.  The Skeleton
#  pages are coded as bytes, the Vorbis pages as Vorbis, and a Vorbis packet
#  that spans a Skeleton page is joined across it.
want skeleton-8k        && enc skeleton-8k        1041 -q 0 -k tiny-8k.wav

#  A minimum bitrate the audio cannot fill: libvorbis pads the audio packets
#  out with zero bits.  Real files from real encoders look like this.
want minbitrate-pad     && enc minbitrate-pad     1042 -m 128  sil44.wav

#  Derived files -- the ways a file can be an Ogg that is not one Vorbis
#  stream from start to end, or not an Ogg at all, one apiece.  Every one
#  of these was once refused; now each has to come back byte for byte, and
#  the interesting number is how much of it still codes as Vorbis.  They
#  are built from tiny-8k-q0.ogg, so they are small and so a diff against
#  them means something, and all of them from one seeded generator, so
#  they are the same bytes on any machine.  Anything with a rewritten page
#  needs its CRC recomputed, or the file would test the CRC path and not
#  the case it was built for -- which is the mistake worth being careful
#  about here.
derived='badcrc-8k.ogg no-eos-8k.ogg id3-prefix-8k.ogg trailing-junk-8k.ogg
         opus.ogg id3-text-8k.ogg id3-apic-8k.ogg id3v1-trailer-8k.ogg
         chained-id3.ogg empty.ogg random.bin page-in-random.bin'
need=0
for n in $derived; do
  [ -f "$n" ] || need=1
done
if [ $force = 1 ] || [ $need = 1 ]; then
  python3 - <<'DERIVED'
import random, struct, zlib

src = open('tiny-8k-q0.ogg', 'rb').read()
other = open('mono-22k-q4.ogg', 'rb').read()
r = random.Random(20240613)

def crc32_ogg(data):
    #  Ogg's CRC-32: polynomial 0x04c11db7, no reflection, no initial or
    #  final inversion.  Not the zlib one, and nothing in the standard
    #  library computes it.
    poly, tbl = 0x04c11db7, []
    for i in range(256):
        c = i << 24
        for _ in range(8):
            c = ((c << 1) ^ poly) & 0xffffffff if c & 0x80000000 else (c << 1) & 0xffffffff
        tbl.append(c)
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

def id3v2(frames):
    #  An ID3v2.3 tag: ten-byte header with a syncsafe size, then frames of
    #  id, big-endian size, two flag bytes, body.  No padding, no footer.
    body = b''.join(fid + struct.pack('>I', len(data)) + b'\0\0' + data
                    for fid, data in frames)
    n = len(body)
    return (b'ID3\x03\x00\x00' + bytes([(n >> 21) & 127, (n >> 14) & 127,
                                        (n >> 7) & 127, n & 127]) + body)

def text(s):  # a text frame body: encoding 0 is ISO-8859-1
    return b'\x00' + s.encode('latin-1')

first = list(pages(src))[0]
last = list(pages(src))[-1]

#  One flipped byte in the last page's payload, with the stored CRC left as
#  it was.  The page is coded as it stands and the CRC it carries with it.
d = bytearray(src); d[last[1] - 1] ^= 0xff
open('badcrc-8k.ogg', 'wb').write(bytes(d))

#  The end-of-stream flag cleared on the last page, and the page re-CRCed so
#  that it is the missing flag that is being tested.  A stream that merely
#  stops is a truncated download, and it has to come back as it is.
d = bytearray(src)
p = bytearray(src[last[0]:last[1]]); p[5] &= 0xfb
d[last[0]:last[1]] = repage(bytes(p))
open('no-eos-8k.ogg', 'wb').write(bytes(d))

#  A bare ID3v2.4 tag in front of the first page -- 42 bytes, nearly all of
#  them zero -- which is the smallest thing a tagger leaves.
open('id3-prefix-8k.ogg', 'wb').write(
    b'ID3\x04\x00\x00\x00\x00\x00\x20' + b'\0' * 32 + src)

#  Junk after the end-of-stream page -- the shape of a file concatenated
#  with something that is not Ogg.
open('trailing-junk-8k.ogg', 'wb').write(src + b'RIFFjunkjunkjunk')

#  Ogg Opus: a well-formed Ogg page whose first packet is an OpusHead.  The
#  container is fine and the codec is not: the page header is modelled and
#  the payload is bytes.
head = (b'OpusHead' + bytes([1, 2]) + struct.pack('<H', 312)
        + struct.pack('<I', 48000) + struct.pack('<h', 0) + bytes([0]))
open('opus.ogg', 'wb').write(repage(
    b'OggS' + bytes([0, 0x02]) + struct.pack('<q', 0)
    + struct.pack('<I', 0x0badface) + struct.pack('<I', 0)
    + struct.pack('<I', 0) + bytes([1, len(head)]) + head))

#  An ID3v2.3 tag with text in it -- title, artist, album, a comment --
#  which is what a tagger really leaves, and bytes the byte model can do
#  something with.
tag = id3v2([(b'TIT2', text('A Short Tone at Four Hundred Hertz')),
             (b'TPE1', text('The Synthesizer')),
             (b'TALB', text('Test Signals, Volume One')),
             (b'TRCK', text('7/12')),
             (b'TYER', text('2024')),
             (b'COMM', b'\x00eng' + b'\x00'
                       + b'Nine hundred samples of sine wave, encoded at '
                         b'quality zero, with this comment in front of them.')])
open('id3-text-8k.ogg', 'wb').write(tag + src)

#  The same with an APIC frame: a 16 by 16 PNG of random pixels, stored
#  rather than deflated (level 0, so the bytes do not depend on the zlib
#  version) -- bytes the byte model can do nothing with, and the shape of
#  the cover art every tagged file carries.
def chunk(kind, data):
    return (struct.pack('>I', len(data)) + kind + data
            + struct.pack('>I', zlib.crc32(kind + data) & 0xffffffff))
rows = b''.join(b'\x00' + bytes(r.randrange(256) for _ in range(16 * 3))
                for _ in range(16))
png = (b'\x89PNG\r\n\x1a\n'
       + chunk(b'IHDR', struct.pack('>IIBBBBB', 16, 16, 8, 2, 0, 0, 0))
       + chunk(b'IDAT', zlib.compress(rows, 0))
       + chunk(b'IEND', b''))
tag = id3v2([(b'TIT2', text('With a Picture')),
             (b'APIC', b'\x00image/png\x00\x03\x00' + png)])
open('id3-apic-8k.ogg', 'wb').write(tag + src)

#  An ID3v1 tag: the 128-byte "TAG" block after the last page, which is
#  what an older tagger leaves and what the older players look for.
def field(s, n):
    return s.encode('latin-1')[:n].ljust(n, b'\0')
open('id3v1-trailer-8k.ogg', 'wb').write(
    src + b'TAG' + field('A Short Tone', 30) + field('The Synthesizer', 30)
    + field('Test Signals', 30) + field('2024', 4) + field('sine, 400 Hz', 30)
    + bytes([12]))

#  Two streams, each behind its own tag: what a cat of two tagged files is.
#  The second tag sits between the streams, so it is junk between links,
#  not junk in front of the file.
open('chained-id3.ogg', 'wb').write(
    id3v2([(b'TIT2', text('First')), (b'TPE1', text('Mono at 22050'))]) + other
    + id3v2([(b'TIT2', text('Second')), (b'TPE1', text('Tiny at 8000'))]) + src)

#  Nothing at all.
open('empty.ogg', 'wb').write(b'')

#  Not Ogg at all: 4 kB from the seeded generator.  Everything about it is
#  the byte model's, and the number to watch is how little it grows.
open('random.bin', 'wb').write(bytes(r.randrange(256) for _ in range(4096)))

#  One valid page -- the first of tiny-8k-q0.ogg, headers and all -- inside
#  random bytes, with a false "OggS" in front of it.  The scan has to find
#  the page and has to not take the four letters for one.
open('page-in-random.bin', 'wb').write(
    bytes(r.randrange(256) for _ in range(1000)) + b'OggS\x01'
    + bytes(r.randrange(256) for _ in range(1000)) + src[first[0]:first[1]]
    + bytes(r.randrange(256) for _ in range(2000)))
DERIVED
  for n in $derived; do echo "  $n  $(wc -c < "$n") bytes"; done
else
  for n in $derived; do echo "  $n  (kept)"; done
fi

rm -f ./*.wav ./bigcomment*.txt ./art.txt
echo "gen.sh: $(ls -1 ./*.ogg ./*.bin | wc -l) files, $(cat ./*.ogg ./*.bin | wc -c) bytes"
