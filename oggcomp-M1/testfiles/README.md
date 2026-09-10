# testfiles/

The corpus `./t.sh` round-trips and `./mk.sh check` codes under both builds.

Every file here is synthesized -- `./testfiles/gen.sh` writes the audio with
python and encodes it with `oggenc` -- so nothing in it is anyone's music and
all of it can be rebuilt.  The encodes fix `--serial`, which oggenc otherwise
picks at random, so rebuilding gives back the same bytes and `git diff` over
this directory means something.  The files are checked in anyway: a fresh
clone can test the compressor with no encoder installed, which is the point.

    ./t.sh                    round-trip all of it
    ./mk.sh check             code all of it under both builds and compare
    ./testfiles/gen.sh -f     build it again from nothing

## What each file is for

| file | what it exercises |
|---|---|
| `sine-stereo-q5.ogg` | two steady tones: nearly all floor, almost no residue |
| `noise-stereo-q3.ogg` | white noise: all residue, and already close to incompressible |
| `mono-22k-q4.ogg` | one channel, and a sample rate that is not 44100 |
| `silence-stereo-q4.ogg` | digital silence -- empty residue partitions, packets that carry no bits |
| `tiny-8k-q0.ogg` | a few pages in total: the headers are most of the file |
| `sweep-mono-qm1.ogg` | a sweep, so no two packets share a floor; `-q -1`, the smallest setup the encoder writes |
| `chirp-stereo-q10.ogg` | `-q 10`: the long blocksize and the widest books |
| `multi6-48k-q4.ogg` | six channels, so 5.1 coupling and six residue vectors per packet |
| `music-stereo-q5.ogg` | the nearest thing here to music, and the size worth watching between revisions |
| `music-managed-b96.ogg` | the same audio bitrate-managed rather than VBR: the encoder pads and truncates packets |
| `bigcomment-8k.ogg` | a 72 kB comment, which cannot fit in one page -- the only file that takes the packet-continuation path |
| `chained.ogg` | two logical bitstreams in one file, with different serials, rates and channel counts |

## refused/

Half of what a lossless compressor promises is that it will not pretend.  A
file it cannot give back byte for byte has to be refused -- loudly, with
nothing written -- rather than coded into a stream that decodes to something
else.  These are the ways of being that file, one apiece; `./t.sh` checks
that each still exits 1.

| file | why it is refused |
|---|---|
| `badcrc.ogg` | one flipped byte in the last page, with the stored CRC left alone |
| `no-eos.ogg` | the end-of-stream flag cleared, and the page re-CRCed so that is what gets noticed |
| `id3-prefix.ogg` | an ID3v2 tag in front of the first page: players skip it, this cannot |
| `trailing-junk.ogg` | not-Ogg after the last page |
| `opus.ogg` | a sound Ogg page whose codec is Opus, not Vorbis |

A build that starts *accepting* one of these is the failure this directory
exists to catch.
