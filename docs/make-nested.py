#!/usr/bin/env python3
"""Build JPEGs whose thumbnails nest as deeply as a JPEG can nest.

    python3 docs/make-nested.py [outdir]      # default: ./nested
    011_/pjpg nested/nest_max.jpg

A JPEG can carry a JPEG thumbnail, and that thumbnail is an ordinary JPEG, so it
can carry one of its own.  Nothing in T.81 or in Exif bounds the depth -- the
only limit is that each level has to fit in its parent's APP segment, whose
length field is 16 bits.  pjpg parses the nesting with one complete parser per
level (PjpgLevels in 011_/pjpg.cpp), so what these files exercise is the
recursion, the handover between levels, and the depth guard at the bottom of it.

There are exactly two ways into the recursion and these files use both,
alternating so that neither path is only ever entered from itself:

  * JFXX APP0 with extension code 0x10, where the rest of the segment is a
    whole JPEG datastream (JFIF 1.02, section 6).
  * Exif APP1 whose IFD1 says Compression (0x0103) = 6, with the thumbnail
    located by JPEGInterchangeFormat (0x0201) and its length (0x0202).

Every level is a valid, decodable baseline JPEG in its own right, so each one
can be pulled out and opened -- the nesting is the only unusual thing about
them.  The images are flat grey: one MCU's worth of all-zero coefficients per
block, which is the smallest conforming baseline datastream there is (~130
bytes) and leaves the segment-length budget to the nesting rather than to
pixels.
"""
import os, struct, sys

# --- a minimal, valid, decodable baseline JPEG ------------------------------
#
# One Huffman table each for DC and AC, both holding a single 1-bit code for
# symbol 0.  Symbol 0 in the DC table is "difference category 0", i.e. a DC
# equal to the previous block's and no extra bits; symbol 0x00 in the AC table
# is EOB.  So every block is two bits, '00', and the whole entropy segment is
# zero bits padded out with ones -- which is also why no 0xFF can appear in it
# and no byte stuffing is needed.

def seg(marker, payload):
    return bytes([0xFF, marker]) + struct.pack('>H', len(payload) + 2) + payload

SOI, EOI = b'\xFF\xD8', b'\xFF\xD9'

JFIF = seg(0xE0, b'JFIF\0' + bytes([1, 2, 0]) + struct.pack('>HH', 1, 1) + bytes([0, 0]))

def baseline_gray(w, h, jfif=True):
    """A w x h greyscale baseline JPEG, all blocks flat."""
    blocks = ((w + 7) // 8) * ((h + 7) // 8)
    bits   = '00' * blocks                      # DC category 0, then EOB
    bits  += '1' * (-len(bits) % 8)             # T.81 F.1.2.3: pad with ones
    data   = bytes(int(bits[i:i+8], 2) for i in range(0, len(bits), 8))
    assert 0xFF not in data                     # all-zero codes cannot make one

    one_code = bytes([1] + [0] * 15) + bytes([0])   # one 1-bit code, symbol 0
    return (SOI
            + (JFIF if jfif else b'')
            + seg(0xDB, bytes([0x00]) + bytes([16] * 64))
            + seg(0xC0, bytes([8]) + struct.pack('>HH', h, w)
                        + bytes([1]) + bytes([1, 0x11, 0x00]))
            + seg(0xC4, bytes([0x00]) + one_code)
            + seg(0xC4, bytes([0x10]) + one_code)
            + seg(0xDA, bytes([1]) + bytes([1, 0x00]) + bytes([0, 63, 0]))
            + data + EOI)

# --- the two ways a JPEG can carry a JPEG -----------------------------------

def exif_app1(thumb):
    """An Exif APP1 whose IFD1 points at `thumb` as a JPEG thumbnail.

    Offsets inside a TIFF block are relative to the byte-order mark, not to the
    segment, so the layout is fixed: IFD0 at 8, IFD1 straight after it, and the
    thumbnail after that.
    """
    def entry(tag, fmt, count, value):          # fmt 3 = SHORT, 4 = LONG
        raw = struct.pack('<H', value) + b'\0\0' if fmt == 3 else struct.pack('<I', value)
        return struct.pack('<HHI', tag, fmt, count) + raw

    IFD0 = 8
    ifd0 = struct.pack('<H', 1) + entry(0x0112, 3, 1, 1)        # Orientation = top-left
    IFD1 = IFD0 + len(ifd0) + 4
    ifd1_body = (entry(0x0103, 3, 1, 6)                          # Compression = JPEG
               + entry(0x0201, 4, 1, 0)                          # patched below
               + entry(0x0202, 4, 1, len(thumb)))
    ifd1 = struct.pack('<H', 3) + ifd1_body
    off  = IFD1 + len(ifd1) + 4                                  # thumbnail starts here
    ifd1 = ifd1.replace(entry(0x0201, 4, 1, 0), entry(0x0201, 4, 1, off))

    tiff = (b'II' + struct.pack('<HI', 0x2A, IFD0)
            + ifd0 + struct.pack('<I', IFD1)
            + ifd1 + struct.pack('<I', 0)
            + thumb)
    assert len(tiff) - len(thumb) == off
    return seg(0xE1, b'Exif\0\0' + tiff)

def jfxx_app0(thumb):
    """A JFXX APP0 carrying `thumb` as its whole payload."""
    return seg(0xE0, b'JFXX\0' + bytes([0x10]) + thumb)

# --- splicing ---------------------------------------------------------------

def insert_after_soi(host, extra):
    return host[:2] + extra + host[2:]

def insert_after_app0(host, extra):
    """JFIF 1.02: a JFXX APP0 comes directly after the JFIF APP0 it extends.

    The length field sits at host[4:6] and counts itself, so the segment ends at
    4 + length -- two bytes further on than the payload does.
    """
    assert host[2:4] == b'\xFF\xE0'
    n = 4 + struct.unpack('>H', host[4:6])[0]
    return host[:n] + extra + host[n:]

def wrap_exif(w, h, thumb):
    # A real Exif file leads with APP1 and has no JFIF APP0 at all.
    return insert_after_soi(baseline_gray(w, h, jfif=False), exif_app1(thumb))

def wrap_jfxx(w, h, thumb):
    return insert_after_app0(baseline_gray(w, h), jfxx_app0(thumb))

# ---------------------------------------------------------------------------

MAX_SEG = 65533          # a segment length field is 16 bits, and counts itself

def build(depth, sizes=None):
    """`depth` thumbnails nested inside one another, innermost first."""
    sizes = sizes or [8 * (depth - i + 1) for i in range(depth + 1)]
    img = baseline_gray(sizes[-1], sizes[-1])
    for lvl in range(depth, 0, -1):
        w = h = sizes[lvl - 1]
        img = wrap_exif(w, h, img) if lvl % 2 else wrap_jfxx(w, h, img)
        assert len(img) < MAX_SEG, 'level %d is too big to embed' % lvl
    return img

out = sys.argv[1] if len(sys.argv) > 1 else 'nested'
os.makedirs(out, exist_ok=True)

# PjpgLevels in 011_/pjpg.cpp: level 0 is the file itself, so this is how many
# thumbnails below it get a parser of their own.
PARSED_LEVELS = 4

manifest = []

def w(name, data, depth, note, chains=1):
    """depth = thumbnails nested in this file; chains = how many such chains.

    The manifest records what a parser should be seen to do with each file, so
    the test does not have to restate it: how many levels it walks (as many as
    it has, up to the guard, per chain) and whether the guard fires.
    """
    open(os.path.join(out, name), 'wb').write(data)
    walked = min(depth, PARSED_LEVELS) * chains
    guard  = chains if depth > PARSED_LEVELS else 0
    manifest.append('%s %d %d %d' % (name, depth, walked, guard))
    print('%-18s %6d bytes  %3d deep   %s' % (name, len(data), depth, note))

# PjpgLevels is 5: level 0 is the file and levels 1..4 are parsed, so the fifth
# thumbnail is the one that meets the guard.  Going past it is the point -- what
# the deep file checks is that the levels below the guard are skipped as opaque
# payload and nothing downstream notices.
w('nest1.jpg',    build(1), 1, 'one thumbnail, Exif')
w('nest2.jpg',    build(2), 2, 'two, Exif then JFXX')
w('nest4.jpg',    build(4), 4, 'the deepest pjpg parses -- the guard stays quiet')
w('nest_max.jpg', build(5), 5, 'one more, so the guard has to fire')
w('nest_deep.jpg',build(8), 8, 'three levels past the guard, all skipped')

# The same nesting reached through one path only, so a failure can be pinned on
# the Exif walk or the JFXX handover rather than on "the recursion".
def build_one_path(depth, exif):
    img = baseline_gray(8, 8)
    for lvl in range(depth, 0, -1):
        s = 8 * (lvl + 1)
        img = wrap_exif(s, s, img) if exif else wrap_jfxx(s, s, img)
    return img

w('nest_exif5.jpg', build_one_path(5, True),  5, 'every level an Exif APP1')
w('nest_jfxx5.jpg', build_one_path(5, False), 5, 'every level a JFXX APP0')

# Two thumbnails on the same image rather than one inside the other.  pjpg has
# one parser per level, not per thumbnail, so the second chain reuses the object
# the first one finished with -- which is the only thing in the recursion that
# depends on a parser being reusable at all.
def build_forked(inner):
    """Returns the image and how deep each of its two chains runs.

    Each thumbnail is itself `inner` deep, and being a thumbnail is one more
    level again, so the chains are inner+1 -- deep enough here that the guard
    has to fire once per chain rather than once per file.
    """
    host = baseline_gray(64, 64)
    host = insert_after_app0(host, jfxx_app0(build_one_path(inner, False)))
    return insert_after_soi(host, exif_app1(build_one_path(inner, True))), inner + 1

forked, fdepth = build_forked(4)
w('nest_two.jpg', forked, fdepth, 'two chains side by side, JFXX and Exif', chains=2)

# As deep as a JPEG can nest at all.  Nothing in T.81 or Exif bounds the depth;
# what bounds it is that every level but the outermost has to fit in its
# parent's APP segment, and that length field is 16 bits.  So wrap until the
# next wrap would not fit, and report where that lands.
def build_to_limit():
    # Derived rather than written down, so the numbers cannot drift from the
    # builders: a segment's payload is everything past its marker and its
    # length field, and what is left of it after the wrapper's own header is
    # all the room the thumbnail has.
    exif_room = MAX_SEG - (len(exif_app1(b'')) - 4)
    jfxx_room = MAX_SEG - (len(jfxx_app0(b'')) - 4)
    img, depth = baseline_gray(8, 8), 0
    while True:
        use_exif = (depth % 2 == 1)
        if len(img) > (exif_room if use_exif else jfxx_room): break
        img = wrap_exif(8, 8, img) if use_exif else wrap_jfxx(8, 8, img)
        depth += 1
    return img, depth

deep, n = build_to_limit()
w('nest_limit.jpg', deep, n, 'the most a 16-bit segment length allows')

open(os.path.join(out, 'manifest'), 'w').write('\n'.join(manifest) + '\n')
