#!/usr/bin/env python3
"""Names the algorithm documents spell that the program no longer has.

    python3 tools/unstale.py                  # ALGORITHM.md, algorithm_v2.md
    python3 tools/unstale.py --list           # and what each one is
    python3 tools/unstale.py old/subs1.hpp    # check them against that source

`ALGORITHM.md` and `algorithm_v2.md` describe the program as it stands, body by
body and field by field.  Every rename since they were written has been able to
falsify a sentence in them, and nothing checked: `rename.py` warns about the
renames *it* makes, and says nothing about an edit made by hand.

`MINIMAL-SYNTAX.md` is deliberately not in that list, and the reason is the
rule's own boundary.  It is a record of a round rather than a description of
the program, so it names what the round *removed* -- `__init_symbol_list`
became `SymList::init` and the sentence saying so has to keep both names.  A
check that cannot tell "the program no longer has this" from "the document is
about the program no longer having this" would be answered by deleting the
history, which is the wrong repair.  Pass it on the command line to see the
list anyway: it is tool names, a calling convention, a keyword, and the names
the round replaced -- and a count of them here would be one more measurement
that stops being re-taken, which is what the rest of this file is about.
`CONTEXT-INDEX.md` and `CLEANER.md` are the same boundary from the other side:
they are a review and a plan, so they name what they *propose* -- an accessor,
three helpers -- which the program does not have and should not be reported as
having lost.

So this reads every identifier the two documents put in backticks and asks
whether the source still has it.  A name is known if `subs1.hpp` or `bmf.cpp`
contains it, with or without the `__` prefix this file puts on the decompiled
bodies.

What it found on the day it was written was one table headed `global` listing
seven Hex-Rays globals -- `__byte_44339C` through `__dword_4433A8` -- as the
per-plane descriptor's fields, which is `PlaneDesc` now; a table of coding
constants under their IDA names; and `near_lossless_max`, which is
`plane_desc[0].w12`.  Reading the first of those against the struct is how
`PlaneDesc::predictor` turned out to be the *reference count* and not a
predictor at all.

`HISTORY` below is the other half.  These documents quote what BMF.exe and
Hex-Rays called things on purpose -- "IDA named each word after whatever
constant it saw the word compared against" -- and a check that cannot tell a
deliberate quotation from a stale reference is a check that gets switched off.
The list is explicit and grouped by reason rather than pattern-matched out of
the prose, because a rule whose test is a spelling gets defeated by a spelling:
"was `v58`" and "reads `v58`" differ by one word.

Adding a name here is a claim that the document means it historically.  Not
finding one here is the finding.
"""
import re
import sys

sys.path.insert(0, __file__.rsplit('/', 1)[0])

DOCS = ('ALGORITHM.md', 'algorithm_v2.md')

# Named on purpose.  The reason is the entry: a name whose reason cannot be
# written down does not belong here.
HISTORY = {
    # IDA's names for the range coder's five words, quoted beside the members
    # that replaced them in a table with a `was` column of its own.
    '__n0x800000': "IDA's name for `range`",
    '__n0x7F800000': "IDA's name for `low`",
    '__byte_4456F0': "IDA's name for `cache`",
    '__dword_4456E8': "IDA's name for `pending`",
    '__dword_4456EC': "IDA's name for `bytes`",
    '__n0x2000': 'a donor global, never in this source',
    '__n0x2000_0': 'a donor global, never in this source',
    '__n0x2000_1': 'a donor global, never in this source',
    '__n0x88': 'a donor global, never in this source',
    'cumFreq': "the donor range coder's term for a cumulative count",
    'totFreq': "the donor range coder's term for a total",
    # The coding parameters, quoted at their BMF.exe addresses in the sentence
    # that says what they became.
    '__dword_44108C': 'was `-F`, now `opt_use_filters`',
    '__dword_441090': 'was `-S`, now `opt_slow`',
    '__n2_4': 'was `-T`, now `opt_filter_template`',
    '__n7_0': 'was `-Q`, now `opt_search_quality`',
    '__n7_1': 'was `-E`, now `opt_max_error`',
    'near_lossless_max': 'the global that became `plane_desc[0].w12`',
    # Code that is not in this source at all: the fast back end and the SIMD
    # the port has no equivalent for.
    'sub_408510': 'the fast back end, deleted (section 7.1)',
    'sub_40CF80': 'the fast back end, deleted (section 7.1)',
    'sub_40F450': 'the fast back end, deleted (section 7.1)',
    'sub_4111B0': 'the predictor-mode-0 encoder, deleted',
    'sub_40A8A0': "named as gone, in the section that says so",
    'log_two_lane': "BMF.exe's two-lane `log`, gone with the SIMD",
    '__bmf_half_half': 'the constant that helper substituted',
    '_mm_andnot_ps': 'an intrinsic, in a sentence about the original',
    # Hex-Rays locals, quoted as what Hex-Rays called them.
    'v58': 'quoted as a Hex-Rays name',
    'n15_2': 'quoted as a Hex-Rays name',
    'n15_3': 'quoted as a Hex-Rays name',
    'n15_4': 'quoted as a Hex-Rays name',
    'n15_7': 'quoted as a Hex-Rays name',
    'n1840': 'quoted as a Hex-Rays name',
    'n960': 'quoted as a Hex-Rays name',
    # Prose in backticks: informal type names in layout tables, and English
    # words the documents emphasise.
    'uint8': 'an informal type name in a layout table',
    'uint16': 'an informal type name in a layout table',
    'uint32': 'an informal type name in a layout table',
    'int32': 'an informal type name in a layout table',
    # The corpus.  `ALGORITHM.md`'s table of coding paths is indexed by the
    # test image each path was observed on, and those are filenames in
    # `testfiles/`, not names in the program.  They are listed one by one
    # rather than matched by shape: a rule like "a name in the table column"
    # would also excuse the next real name that lands there.
    'f05_200': 'testfiles/f05_200.bmp',
    'DLRAW': 'testfiles/DLRAW.bmp',
    'rle4': 'testfiles/out_rle4.bmp',
    'rle8': 'testfiles/out_rle8.bmp',
    'x_ai': 'testfiles/x_ai.bmp',
    'x_ci': 'testfiles/x_ci.bmp',
    'x_ep': 'testfiles/x_ep.bmp',
    't8g': 'testfiles/t8g.bmp',
    't8p': 'testfiles/t8p.bmp',
    't24': 'testfiles/t24.bmp',
    't32': 'testfiles/t32.bmp',
    'med32': 'testfiles/med32.bmp',
    'altp1': 'testfiles/altp1.bmp',
    'noise24': 'testfiles/noise24.bmp',
    'xform1': 'testfiles/xform1.bmp',
    'xform2': 'testfiles/xform2.bmp',
    # The BMP file format's own vocabulary.  The fields this one names --
    # `biCompression`, `biHeight` -- are members of `BmpHeader` and are found;
    # the structure's name is Microsoft's and is not in the source at all.
    'BITMAPINFOHEADER': "the BMP header's name in the Windows headers",
    # `bcdr.cpp`, the 1bpp compressor read alongside this one.  The sentences
    # quoting it are about the other program on purpose -- that is what makes
    # them evidence for how this one's counters were meant to work.
    'BIN_CODER': "bcdr.cpp's binary coder, quoted as the donor",
    'scaleRare': "bcdr.cpp's name for the rare-symbol rescale",
    'was': 'a table column heading',
    'old': 'an English word, emphasised',
    'length': 'an English word, emphasised',
    'index': 'an English word, emphasised',
    'coded': 'an English word, emphasised',
}


BODY_LINES = re.compile(r'`(\w+)`[^`\n]{0,60}?\(?(\d[\d ,]*) lines')


def read(src):
    """The source as the compiler reads it: `#include "x.inc"` spliced in.

    `bmf.cpp` is an include list, so a check that opens it sees thirty-seven
    filenames and no program.  Reading it as one file said a hundred names the
    documents use -- `SymList`, `__colour_transform`, `PixRec` -- were names
    the program no longer has, all of them one `#include` away.
    """
    import structs
    try:
        return structs.splice(src)[0]
    except IOError:
        return None


def sizes(sources):
    """{body name: line count} for every body in the source."""
    import structs
    out = {}
    for src in sources:
        if not src.endswith(('.hpp', '.cpp')):
            continue
        lines = read(src)
        if lines is None:
            continue
        # `structs.defs`, not `structs.bodies`: a method is a body the
        # documents quote and count, and `bodies` stops at brace depth 0.
        for a, b, nm, _sig, _d in structs.defs(lines):
            out[nm.lstrip('_')] = b - a + 1
    return out


def measured(docs, sources, slack=2):
    """Claims of the form ``X` is N lines` that the source disagrees with.

    A name that is gone is one kind of stale; a number that has moved is the
    other, and `ALGORITHM.md` had three -- `code_pixel` at 787 when it is 781,
    `alt_p2_model` at 1969 when it is 1402, and "all 71 names" against an
    address map that has 74.  A sentence arguing that a body is too long to
    read in one sitting does not become wrong when it loses six lines, so the
    check has slack; it is there for the ones that have drifted by hundreds.
    """
    have, out = sizes(sources), []
    for doc in docs:
        try:
            lines = open(doc).read().split('\n')
        except IOError:
            continue
        for i, line in enumerate(lines, 1):
            for m in BODY_LINES.finditer(line):
                nm, n = m.group(1), int(m.group(2).replace(' ', '').replace(',', ''))
                if nm in have and abs(have[nm] - n) > slack:
                    out.append((doc, i, nm, n, have[nm]))
    return out


def known(names, n):
    """Is `n` an identifier the source still has?

    The bodies carry a `__` prefix here and the documents write them without
    one, so `compress_image` and `__compress_image` are the same name.
    """
    return n in names or ('__' + n) in names or n.lstrip('_') in names


def survey(docs=DOCS, sources=('subs1.hpp', 'bmf.cpp')):
    text = []
    for p in sources:
        lines = read(p)
        if lines is not None:
            text.append('\n'.join(lines))
    code = '\n'.join(l.split('//')[0] for l in '\n'.join(text).split('\n'))
    names = set(re.findall(r'[A-Za-z_]\w*', code))
    out = []
    for doc in docs:
        try:
            lines = open(doc).read().split('\n')
        except IOError:
            continue
        for i, line in enumerate(lines, 1):
            for m in re.finditer(r'`([A-Za-z_]\w*)`', line):
                n = m.group(1)
                if len(n) <= 2 or known(names, n) or n in HISTORY:
                    continue
                out.append((doc, i, n, line.strip()))
    return out


def main():
    # `sweep.sh` hands every tool a copy of subs1.hpp and `proven.sh` hands it
    # old revisions of one.  Neither is a mistake and neither should be
    # ignored: the documents are checked *against a source*, so a source given
    # on the command line is the source to check them against.  Ignoring it was
    # what made this answer identically for every revision -- "never opens the
    # file it is given", which is the one verdict `reads.py` exists to print.
    docs = [a for a in sys.argv[1:] if a.endswith('.md')] or list(DOCS)
    src = [a for a in sys.argv[1:]
           if not a.endswith('.md') and not a.startswith('--')]
    found = survey(tuple(docs), tuple(src) + ('bmf.cpp',) if src
                   else ('subs1.hpp', 'bmf.cpp'))
    seen = set()
    for doc, line, name, text in found:
        if '--list' in sys.argv or name not in seen:
            print('%-22s %s:%d  %s' % (name, doc, line, text[:70]))
        seen.add(name)
    sources = tuple(src) + ('bmf.cpp',) if src else ('subs1.hpp', 'bmf.cpp')
    drift = measured(docs, sources)
    for doc, line, nm, said, is_ in drift:
        print('%-22s %s:%d  says %d lines, is %d' % (nm, doc, line, said, is_))
    print('%d names in %d documents that the program no longer has, '
          '%d line counts that have drifted'
          % (len(seen), len(set(d for d, _, _, _ in found)), len(drift)))
    return 0


if __name__ == '__main__':
    sys.exit(main())
