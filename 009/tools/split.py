#!/usr/bin/env python3
"""Split bmf.cpp into one file per class and per large function.

    python3 tools/split.py            # what it would write
    python3 tools/split.py --apply    # write it

Kept for the reason the six migrations at the top of README.md are kept: the
decompilation is generator output, and if it is produced again from the
extractor as one file this is what has to be replayed on top of it.  It is not
idempotent -- run against an already split tree it finds a bmf.cpp that is an
include list and reports that the layout names things bmf.cpp does not define.

What it does, and what each part of it cost:

  * every top-level definition is assigned to a file by name, in `LAYOUT`, and
    a name that is in neither `LAYOUT` nor `KEEP` stops the run.  An assignment
    by pattern -- "everything matching `__alt_p2_*`" -- would have quietly
    dropped whatever the pattern did not reach.

  * a method's body replaces its declaration inside its class, indented by two,
    which is the whole of "define methods in classes".  The classes form a DAG
    -- only `ModelBlock` calls another class's methods -- so all 22 can move;
    a cycle would have meant leaving one side out of line.

  * `decls.inc` declares every function before any of them is defined, which is
    what lets `LAYOUT` be a reading order.  Without it the file has exactly one
    legal order, the one the decompiler emitted.

  * nothing may sit between two top-level items but blanks, comments and the
    preprocessor, and `items()` fails if anything does.  That check is here
    because `#pragma pack(push, 1)` did: the first run left `SymEntry` and
    `BmpHeader` unpacked, which compiles and reads a BMP header at the wrong
    offsets.  `wrappers()` gives the pragmas back to the struct they wrap.

  * `BMF_SPILL_PAD` is a `#define` between items and belongs with the rest of
    the decompiler's vocabulary in `ida.inc`; it is placed by name below.

  * units written by hand *after* the split are not in `LAYOUT` and this does
    not know about them -- `ctxidx.inc` is one.  A replay produces the tree
    without them and their includes have to be put back.

The gate is what says whether a run was right.  This one was checked by taking
the multiset of non-comment lines before and after: 44 lines go (22 method
declarations, 22 out-of-class heads) and 138 arrive (22 in-class heads, 34
includes, 21 forward declarations, 61 prototypes), and no line of program logic
is touched.
"""
import re
import sys
import os

SRC = 'bmf.cpp'

# ---------------------------------------------------------------- parse ----
def items(lines):
    """(start, end, head) for every top-level definition, 1-based inclusive."""
    out = []
    depth = 0
    start = None
    head = None
    for i, ln in enumerate(lines, 1):
        if depth == 0 and start is None and ln[:1] not in ('', ' ', '\t', '#', '}'):
            start, head = i, ln
        depth += ln.count('{') - ln.count('}')
        if start and depth == 0 and (ln.endswith('}') or ln.endswith(';') or ln.endswith('};')):
            out.append((start, i, head))
            start = None
    assert start is None, 'unterminated item at line %d' % start
    # Nothing may fall between the items but blank lines, comments and the
    # preprocessor.  A `#pragma pack(push, 1)` did, and `SymEntry` and
    # `BmpHeader` came out of the split unpacked -- which compiles, and reads
    # the BMP header at the wrong offsets.  The check is here rather than in a
    # comment because that is the whole class of defect a split can have.
    covered = set()
    for s, e, _ in out:
        covered.update(range(s, e + 1))
    stray = [(i, ln) for i, ln in enumerate(lines, 1)
             if i not in covered and ln.strip() and not ln.startswith(('#', '//'))]
    if stray:
        raise SystemExit('outside every item: ' +
                         '; '.join('%d: %s' % x for x in stray[:5]))
    return out


def wrappers(lines, its):
    """{item index: (before, after)} for the preprocessor lines that wrap it.

    `#pragma pack(push, 1)` belongs to the struct after it and `pop` to the
    struct before it; a `#define` between items belongs to nothing and is
    reported so it can be placed by hand.
    """
    starts = {s: k for k, (s, e, h) in enumerate(its)}
    ends = {e: k for k, (s, e, h) in enumerate(its)}
    wrap = {}
    for i, ln in enumerate(lines, 1):
        if not ln.startswith('#pragma'):
            continue
        if 'push' in ln:
            nxt = next((s for s in sorted(starts) if s > i), None)
            wrap.setdefault(starts[nxt], [[], []])[0].append(ln)
        else:
            prv = max(e for e in ends if e < i)
            wrap.setdefault(ends[prv], [[], []])[1].append(ln)
    return wrap


def name_of(head):
    """The identifier a top-level item defines."""
    # `alignas(16)` and `__attribute__((noreturn))` carry parentheses of their
    # own, so anything that reads the first `(` as the argument list calls half
    # the tables `alignas`.  Strip them first.
    h = head
    while True:
        m = re.match(r'^(?:__attribute__\(\(.*?\)\)|alignas\(\d+\))\s*', h)
        if not m:
            break
        h = h[m.end():]
    m = re.match(r'^(?:struct|class|union) (\w+)', h)
    if m:
        return m.group(1)
    m = re.match(r'^inline\s+.*?\b(\w+)::(\w+)\s*\(', h)
    if m:
        return m.group(1) + '::' + m.group(2)
    # `static uint8_t(&__byte_445440)[544] = *(uint8_t(*)[544])(bss_exclusion+8192);`
    # -- the name is inside the declarator, and the initialiser ends in `);`,
    # which is exactly the shape the function branch below reads.  Asked first,
    # and only of something that is not a definition, since a function can take
    # a reference to an array too.
    if not h.rstrip().endswith('{'):
        m = re.search(r'\(\s*&\s*(\w+)\s*\)', h)
        if m:
            return m.group(1)
    if '(' in h and (h.rstrip().endswith('{') or re.match(r'^.*?\b\w+\s*\(.*\)\s*;$', h)):
        # a function: the identifier that ends the text before the arguments
        m = re.search(r'(\w+)$', h.split('(')[0].strip())
        if m:
            return m.group(1)
    # an ordinary variable: the identifier an initialiser, a subscript or the
    # semicolon follows
    m = re.match(r'^.*?\b(\w+)\s*(?:\[|=|;)', h)
    return m.group(1) if m else head


# ------------------------------------------------------------ the layout ---
# Every item lands in exactly one file.  Order here is the order bmf.cpp
# includes them, and it is a dependency order: globals before the coder,
# the coder before the classes that use `rc`, the classes before the
# functions, `ModelBlock` after the two classes its methods call.
LAYOUT = [
 ('ida.inc',        ['abs32', '__OFSUB__']),
 ('tables.inc',     ['ctx_group_flags', 'p2_ctx_rotate', 'p2_ctx_edges', 'p2_len_edges',
                     'bmf_p2_thresholds', 'p2_float_pool', 'p1_level_edges', 'p1_group_edges',
                     'p1_slot_edges', 'p1_level_step', 'bmf_p2_coef', 'bmf_p2_rate',
                     'bmf_p2_rate_reset', 'bmf_p2_ms_rate', 'bmf_p2_mix', 'bmf_p2_decay',
                     'bmf_p2_seed']),
 ('globals.inc',    ['__exit_402E40', 'coded_block', 'plane_predictor', 'plane_alt_model',
                     'packer_free_bits', 'packer_acc', 'coded_size', 'desc_slow_mode',
                     'alphabet_reduced', 'PlaneDesc', 'plane_desc', 'plane_count',
                     'near_lossless_q', 'model_geometry', 'exclusion_gen', 'alt_freq_limit',
                     'alt_freq_init', 'LevelGeom', 'level_geom', 'ctx_bias', 'deadzone_hi',
                     'deadzone_lo', 'opt_use_filters', 'opt_slow', 'opt_filter_template',
                     'opt_pack_output', 'opt_search_quality', 'opt_max_error', 'packer_word',
                     'out_cursor', 'coded_buf', 'hist_scratch', 'bss_exclusion',
                     'exclusion_mask', '__byte_445440', 'model_tables', 'model_strip',
                     'mode_symbol', 'bmf_zero16', 'bmf_plane_desc', 'bmf_hsum4', 'bmf_set_denormal_mode', '__set_new_handler',
                     '__out_of_memory_handler']),
 ('rc.inc',         ['rc']),                       # rc.inc exists; `rc` joins it
 ('decls.inc',      []),                           # generated
 ('symlist.inc',    ['FreqRec', 'SymEntry', 'SymList', 'SymList::rescale',
                     'SymList::code_symbol', 'SymList::add_weight', 'SymListBlock',
                     'free_sym_entries', 'sym_list_block', 'sym_list_count', 'sym_in_top',
                     '__init_symbol_list']),
 ('bitctr.inc',     ['SymPair', 'FreqPair', 'bit_tree', 'BitCtr', 'BitCtr::encode_context_bit',
                     'BitCtr::decode_context_bit', '__rescale_counter_pair',
                     'CounterNode', '__init_counter_node']),
 ('altp1.inc',      ['CtxWeight', 'P1Ctx', 'AltP1Block', 'AltP1Block::ctx_of',
                     'AltP1Block::update_model', 'AltP1Block::alt_p1_alloc',
                     'AltP1Block::alt_p1_free', 'AltP1Block::d8_encode_body']),
 ('altp2.inc',      ['P2Count', 'P2Freq', 'P2Freq::encode_symbol', 'P2Freq::decode_symbol',
                     'P2Freq::rescale_three_way', 'p2_pred', 'p2_bump', 'P2Ctx', 'AltP2Block',
                     'AltP2Block::alt_p2_alloc', 'AltP2Block::alt_p2_free', 'CtxWeights']),
 ('model.inc',      ['PixRec', 'ModelBlock', 'ModelBlock::pixel_context',
                     'ModelBlock::init_tables', 'ModelBlock::decode_pixel',
                     'ModelBlock::code_pixel', 'ModelBlock::expand_alphabet',
                     'ModelBlock::unmodel_plane_slow']),
 ('bmp.inc',        ['BmfImage', 'BmpHeader', 'bmf_pixels', '__alloc_image']),
 ('arc.inc',        ['BmfArc', 'BmfArc::bmf_close_archive', '__bmf_open_archive',
                     '__bmf_destroy_archive', '__bytes_left']),
 # the range coder's file-level ends, which are not the coder itself
 ('rcio.inc',       ['__rc_decode_flat', '__rc_end_decode', '__rc_end_encode',
                     '__rc_begin_encode', '__rc_begin_decode', '__packer_word']),
 ('symcode.inc',    ['__encode_symbol_tree', '__decode_symbol_tree', '__alt_p1_encode_symbol',
                     '__alt_p1_decode_symbol', '__update_binary_pair', '__estimate_cost']),
 ('decode_symbol_list.inc', ['__decode_symbol_list']),
 ('workspace.inc',  ['__free_workspace', '__layout_workspace']),
 ('reduce_alphabet.inc',    ['__reduce_alphabet']),
 ('p1.inc',         ['__alt_init_tables', '__alt_model_p1_d8_encode', '__alt_model_p1_d8_decode']),
 ('alt_model_p1_encode.inc', ['__alt_model_p1_encode']),
 ('alt_model_p1_decode.inc', ['__alt_model_p1_decode']),
 ('p2.inc',         ['__alt_p2_filter', '__alt_p2_d8_decode_body', '__alt_p2_d8_encode_body',
                     '__alt_model_p2_d8_encode', '__alt_model_p2_d8_decode']),
 ('alt_p2_context.inc',      ['__alt_p2_context']),
 ('alt_p2_model.inc',        ['__alt_p2_model']),
 ('alt_model_p2_encode.inc', ['__alt_model_p2_encode']),
 ('alt_model_p2_decode.inc', ['__alt_model_p2_decode']),
 ('predict.inc',    ['__predict_med', '__unpredict_med', '__colour_transform',
                     '__interleave_plane', '__transform_planes', '__expand_predictor_mode0']),
 ('plane.inc',      ['__model_planes', '__unmodel_plane']),
 ('model_plane.inc',         ['__model_plane']),
 ('cost_candidate.inc',      ['__cost_candidate']),
 ('choose_plane_coding.inc', ['__choose_plane_coding']),
 ('search_filter.inc',       ['__search_filter']),
 ('read_bmp.inc',            ['__read_bmp']),
 ('write_bmp.inc',           ['__write_bmp']),
 ('expand_image.inc',        ['__expand_image']),
 ('compress_image.inc',      ['__compress_image']),
]

# What stays in bmf.cpp: the entry points.
KEEP = ['__bmf_compress', '__bmf_decompress', '__main', 'main']


def main():
    lines = open(SRC).read().split('\n')
    its = items(lines)
    by_name = {}
    for s, e, h in its:
        n = name_of(h)
        by_name.setdefault(n, []).append((s, e, h))

    assigned = {}
    for fn, names in LAYOUT:
        for n in names:
            if n not in by_name:
                sys.exit('layout names %r, which bmf.cpp does not define' % n)
            assigned[n] = fn
    for n in KEEP:
        assigned[n] = None

    missing = [name_of(h) for s, e, h in its if name_of(h) not in assigned]
    if missing:
        sys.exit('unassigned: %s' % ', '.join(sorted(set(missing))))

    # ------------------------------------------------ methods, in-class ----
    # A method's body replaces its declaration inside the struct, indented by
    # two.  Both spellings of the head have to be handled: `inline T C::m(` and
    # `inline T*C::m(` -- the decompilation writes the star against the name.
    methods = {}
    for s, e, h in its:
        n = name_of(h)
        if '::' in n:
            cls, meth = n.split('::')
            body = lines[s - 1:e]
            head = re.sub(r'^inline\s+(.*?)\b%s::' % cls, r'\1', body[0])
            methods.setdefault(cls, {})[meth] = ['  ' + head] + \
                ['  ' + x if x else '' for x in body[1:]]

    def struct_text(s, e, cls):
        """The struct, with each declared method replaced by its definition."""
        out = []
        for ln in lines[s - 1:e]:
            m = re.match(r'^  .*?\b(\w+)\(.*\);$', ln)
            if m and cls in methods and m.group(1) in methods[cls]:
                out.extend(methods[cls].pop(m.group(1)))
                out.append('')
            else:
                out.append(ln)
        while out and out[-2:] == ['', '};']:
            del out[-2]
        return out

    # ------------------------------------------------------ prototypes -----
    # Every non-static function defined after the classes, declared before
    # them.  This is what lets the layout above be a reading order instead of
    # a dependency order: a body can call anything, wherever it ends up.
    proto = []
    seen = set()
    for s, e, h in its:
        n = name_of(h)
        if '::' in n or n in KEEP or n in seen:
            continue
        if not h.rstrip().endswith('{') or '(' not in h.split('(')[0] + '(':
            pass
        if not h.rstrip().endswith('{'):
            continue                                   # not a definition
        if '(' not in h or h.startswith('struct '):
            continue                                   # not a function
        # The three files below are included before this one, so their
        # definitions are their own declarations.  `static` is kept where it
        # is: a static function declared without it has external linkage and
        # the definition that follows is a different function.
        if assigned.get(n) in ('ida.inc', 'globals.inc', 'tables.inc'):
            continue
        seen.add(n)
        proto.append(re.sub(r'\s*\{$', ';', h))

    fwd = ['struct %s;' % name_of(h) for s, e, h in its
           if h.startswith('struct ') and assigned.get(name_of(h)) not in
           ('globals.inc', 'tables.inc')]

    # --------------------------------------------------------- emit --------
    HEADERS = {}
    for fn, _ in LAYOUT:
        HEADERS[fn] = []

    wrap = wrappers(lines, its)
    for k, (s, e, h) in enumerate(its):
        n = name_of(h)
        fn = assigned[n]
        if fn is None or '::' in n:
            continue
        before, after = wrap.get(k, ([], []))
        HEADERS[fn].extend(before)
        if h.startswith('struct ') and n in methods:
            HEADERS[fn].extend(struct_text(s, e, n))
        else:
            HEADERS[fn].extend(lines[s - 1:e])
        HEADERS[fn].extend(after)
        HEADERS[fn].append('')

    for cls, left in methods.items():
        if left:
            sys.exit('%s: no declaration for %s' % (cls, ', '.join(left)))

    HEADERS['decls.inc'] = fwd + [''] + proto
    return HEADERS, lines, its, assigned


WHAT = {
 'ida.inc': 'the Hex-Rays vocabulary -- the macros the decompiler writes, and the two\nhelpers it calls by name',
 'tables.inc': "the constant tables, in the order they sit in BMF.exe's data segment",
 'globals.inc': "the program's state, and the exit codes it answers with.  The small\nstructs are here rather than with the classes because globals are declared in\nterms of them",
 'decls.inc': 'every function this program defines, declared before any of it is.\n\nThis is what lets the include list in bmf.cpp be a reading order instead of a\ndependency order: a body can call anything, wherever it ends up, and a class\ncan define its methods in its own body without waiting for what they call.\nWithout it the file has exactly one legal order -- the one the decompiler\nemitted -- and every one of these units would have to sit where its callers\nput it',
 'symlist.inc': 'the symbol list: the alphabet, its weights, and the search that\ncodes one symbol of it',
 'bitctr.inc': 'the binary counters -- one context bit, coded and decoded, and the\npair rescale behind them',
 'altp1.inc': 'the alternate model for one-bit planes',
 'altp2.inc': 'the alternate model for deep planes: its counters, its frequency\nrecords and the block they live in',
 'model.inc': 'the main model.  One pixel in and one pixel out are the two largest\nbodies in the program, and they are methods of the block they read',
 'bmp.inc': 'the BMP file, as this program holds it in memory',
 'arc.inc': 'the archive: several images in one file, which is what appending a\nmember to an existing output makes',
 'rcio.inc': "the range coder's file ends -- what it writes before a stream and what\nit reads after one.  The coder itself is rc.inc",
 'symcode.inc': 'the symbol trees: a symbol through a tree of binary counters, both\nways, for both models',
 'decode_symbol_list.inc': 'read back the alphabet an encoder wrote',
 'workspace.inc': "the main model's workspace: how it is laid out and how it is given\nback",
 'reduce_alphabet.inc': 'drop the symbols an image does not use, and renumber what is left',
 'p1.inc': 'the one-bit plane model, around its block',
 'alt_model_p1_encode.inc': 'one-bit planes, encoded',
 'alt_model_p1_decode.inc': 'one-bit planes, decoded',
 'p2.inc': 'the deep-plane model around its block: the filter, and the two\ndirections of its eight-bit body',
 'alt_p2_context.inc': "the deep-plane model's context: which of its counters the next\nsample is coded against",
 'alt_p2_model.inc': 'the deep-plane model itself, and the largest body in the program',
 'alt_model_p2_encode.inc': 'deep planes, encoded',
 'alt_model_p2_decode.inc': 'deep planes, decoded',
 'predict.inc': 'the predictors and the colour transform -- what is subtracted from\nan image before it is modelled, and added back after',
 'plane.inc': 'one plane in and out of the model, whichever model the header names',
 'model_plane.inc': 'one plane through the main model',
 'cost_candidate.inc': 'what one filter would cost on this image',
 'choose_plane_coding.inc': 'decide how each plane of an image is to be coded',
 'search_filter.inc': 'search the filters, which is what -Q9 spends its time on',
 'read_bmp.inc': 'read a BMP, in every depth and both run-length forms',
 'write_bmp.inc': 'write one back',
 'expand_image.inc': 'one member out of an archive',
 'compress_image.inc': 'one image into one',
}

BMF_CPP_HEAD = '''\
// BMF lossless image compressor, v.2.01 (C) 1998-1999, 2009 by Dmitry Shkarin.
// A decompilation of BMF.exe; see tools/README.md for how it is kept honest.
//
// This file is the include list and the entry points.  Everything else is one
// unit per file below: a class with its methods defined in its own body, or a
// function large enough to be read on its own.
//
// The order is a reading order, not a dependency order -- `decls.inc` declares
// every function before any of them is defined, so a unit sits with what it
// belongs to rather than with what happens to call it.  What the order does
// still say is that the four files before `decls.inc` are the ones everything
// is written in terms of: the decompiler's macros, the allocator, the constant
// tables and the program's state.
'''


def emit(HEADERS):
    order = [fn for fn, _ in LAYOUT]
    for fn in order:
        body = HEADERS[fn]
        if fn == 'rc.inc':                      # exists already; `rc` joins it
            old = open(fn).read().rstrip('\n')
            open(fn, 'w').write(old + '\n\n// The one coder the program has.\n'
                                + '\n'.join(x for x in body if x.strip()) + '\n')
            continue
        head = '// %s -- %s.\n' % (fn, WHAT[fn].replace('\n', '\n// '))
        text = '\n'.join(body).strip('\n')
        open(fn, 'w').write(head + '\n' + text + '\n')

    keep = []
    wrap = wrappers(lines, its)
    for k, (s, e, h) in enumerate(its):
        if assigned[name_of(h)] is None:
            before, after = wrap.get(k, ([], []))
            keep.extend(before + lines[s - 1:e] + after + [''])
    incs = '\n'.join('#include "%s"' % fn for fn in
                     ['ida.inc', 'memory.inc'] + [f for f, _ in LAYOUT
                                                  if f not in ('ida.inc',)])
    open(SRC, 'w').write(BMF_CPP_HEAD + '\n'
                         + '\n'.join(lines[0:9]) + '\n\n'
                         + incs + '\n\n'
                         + '\n'.join(keep).strip('\n') + '\n')


if __name__ == '__main__':
    HEADERS, lines, its, assigned = main()
    spill = next(x for x in lines if x.startswith('#define BMF_SPILL_PAD'))
    HEADERS['ida.inc'] = lines[9:20] + ['', spill, ''] + HEADERS['ida.inc']
    if '--apply' in sys.argv:
        emit(HEADERS)
        print('written')
    for fn, body in HEADERS.items():
        print('%-28s %5d lines' % (fn, len(body)))
