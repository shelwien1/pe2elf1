# pdfdump — compression plan, round 2

Follow-up to `pdfdump_plan.md`. Everything below is measured on the README test set (120 PDFs:
117 files of the 005 set + the 3 supplied ones, 55.6 MB) with the current build, `xz -9`.
"Exact" means the real encoder produced the alternative stream; "prototype" means a Python model
of the encoder (`tests/csproto.py`) whose baseline lands within 2% of the real one.

## 1. Where the bytes are

PDF-specific output (everything except the payload formats that go to other recompressors):
1543 files, 8.46 MB as one tar.xz.

| Output | Holds | xz | Share |
|---|---|---|---|
| `.csn` | numbers of content streams | 3117 KB | 37% |
| `.cs` | content-stream skeleton + literal strings | 2066 KB | 24% |
| `.t1d` | decrypted Type 1 fonts | 1228 KB (fonts of a PDF in file order) | 15% |
| `.rec` | recon records | 422 KB | 5% |
| `.txn` | numbers of object text | 388 KB | 5% |
| `.txt` | object text | 383 KB | 5% |
| `.csh` | hex strings of content streams | 336 KB | 4% |
| `.bin` | small binary streams, XRef row residuals | 286 KB | 3% |
| `.ai` `.meta` `.xml` `.cmap` `.cmb` `.ps` | | 354 KB | 4% |

The per-type figures are xz of each file on its own, so they add up to slightly more than the
tar. Content streams are 65% of the total, Type 1 fonts 15%, all object-level syntax together 13%.
The earlier rounds worked mostly on the 13%.

Content-stream numbers by operator (xz of that operator's numbers over all files):

| Operator | Share of `.csn` |
|---|---|
| `TJ` (kerning inside arrays) | 30.5% |
| `c` | 13.8% |
| `Td` | 13.5% |
| `l` | 11.4% |
| `Tm` | 7.6% |
| `m` | 5.4% |
| `TD` | 4.5% |
| `cm` | 3.9% |
| `re` | 3.6% |
| `Tc`, `Tw` | 3.7% |
| everything else | 2.1% |

Text positioning is 60%, path construction 34%. No file dominates: the 12 largest `.csn`
files hold 47%.

## 2. The four questions

**Remaining computable values**

| Value | Cost now | Finding |
|---|---|---|
| Number of decimals of each lifted number | 3 bits in every value | computable from a per-group unit: P1, −241 KB exact |
| `Td`/`TD` advance after shown text | 18% of `.csn` | predictable from a table learned on the fly: P2, −7.1% of `.csn` (prototype) |
| Path coordinates | 34% of `.csn` | residual against the current point: P3, −3.3% of `.csn` (prototype) |
| Item text lengths (channel N) | 54 KB | derivable by lexing to `endobj`: P7, ceiling 54 KB |
| `/Widths`, `/W` | 44.5 KB of `.txn` | derivable from the embedded font program; three font formats to parse: P8 |
| Structure tree `/K` `/P` `/Nums` | 49 KB of `.txn` | `/ParentTree` is the inverse of `/K`; ceiling 49 KB: P8 |
| `/CharSet`, CIDSet | 12 KB + 8 KB raw | derivable from the font; too small |
| `/Rect`, `/D` (destinations) | 91 KB of `.txn` | not computable without layout |
| `TJ` kerning | 30.5% of `.csn` | a table per (font, glyph before, glyph after) is worse: +2.5% (only 41% of hits exact) |

**Duplicate records**

- One 3144-byte sRGB ICC profile is embedded in 27 of the 120 PDFs (2500 bytes of xz each), a
  2008-byte one in 2 more: 69.5 KB of payload xz. A built-in table of well-known profiles
  removes it: P5.
- Info dates repeated in the XMP packet: 49 of 182 dates, about 15 bytes each. Not worth a
  mechanism.
- Fonts identical across PDFs: 4 TrueType, 1 CFF, 3 Type 1 in 120 files. Subsets differ; nothing
  to do per file.
- BMP headers of the 923 small images in `.bin` repeat the dictionary's width and height, but
  they pay for themselves through the derived `/Width` `/Height`: raw samples instead cost
  +3.7 KB.
- Streams with identical stored form are already stored once; repeats inside the text files are
  within xz's reach.

**Mistyped data**

- Literal strings sit inside the `.cs` skeleton: 5.86 MB of its 32.4 MB raw, 1.43 MB of its
  2.07 MB xz, often raw binary glyph codes (one file's `.cs` is 22% printable). Moving them to a
  stream of their own: −88 KB (−4.3% of `.cs`), P4.
- The XMP thumbnail layer keeps the XMP text around the thumbnail in `.rec` (557 KB raw in 26
  streams). It costs 22.1 KB there against 21.1 KB as a text file: 1 KB, left alone.
- `.ps` (PostScript XObjects of one file) holds 58 KB of hex data; 14.7 KB of xz in all.
- `.bin` is 29% printable and that is image and XRef data, no text. The `.txt` files with the
  lowest printable share are low because of number markers, not binary. `.ai` is 100% text here
  (no compressed Illustrator blocks).

**Undecoded strings**

| Encoding | Bytes left | Finding |
|---|---|---|
| Octal escapes in content-stream strings | 385 KB raw | unescaping to bytes is worse: +3.2% of the strings stream |
| Hex strings in object text | 48 KB raw | as binary side stream: +1.0% of `.txt` |
| Literal strings in object text | 907 KB raw | as a side stream: −0.8% of `.txt` (9 KB) |
| Base64 | none | the "base64-looking" runs in `.txt` are `/CharSet` and `/Differences` name lists and palette hex strings; over the 808-file regression set only XMP thumbnails matched, and those are extracted |
| XML character references, URL escapes, `#xx` in names | under 4 KB each | nothing to gain |
| UTF-16BE text strings | 2.5 KB | earlier measurement: −0.7 KB |
| Streams left encoded | CCITT 373 streams / 144 KB (by design, `.tif`), Flate 5 streams / 6 KB | nothing new |

## 3. Work items

| # | Item | Gain | Evidence | Effort |
|---|---|---|---|---|
| P1 | One decimal unit per number group | `.csn` 3117 → 2875 KB (−7.7%), `.txn` −0.5 KB | exact | small |
| P2 | `Td`/`TD` advance from a learned table | −7.1% of `.csn` (about −200 KB) | prototype | medium |
| P3 | Path coordinates against the current point | −3.3% of `.csn` (about −93 KB) | prototype | medium |
| P4 | Literal strings out of the `.cs` skeleton | `.cs` 2066 → 1978 KB (−4.3%) | exact split, Python | small |
| P5 | Table of well-known ICC profiles | −69.5 KB of payload xz | exact count | small |
| P6 | Type 1 charstring operands per operator | unknown; operands are 910 KB of the 1228 KB | experiment first | medium |
| P7 | Item lengths derived by lexing | at most −54 KB | ceiling | medium |
| P8 | `/Widths`, `/W`, `/ParentTree` from other data | at most −44 KB and −49 KB | ceilings | large |

P1 to P4 together: about −620 KB, 7.3% of the PDF-specific total. All four are linear passes
over data the lifter already walks, so `c` speed is unaffected.

### P1. One decimal unit per number group

Now every lifted number is `zigzag(mantissa) * 8 + code`, code = number of fraction digits. In a
group such as the x operand of `Td`, "12.5", "13" and "14.25" get mantissas 125, 13 and 1425,
which breaks the delta mode and spends 3 bits per value on the code.

- Per group: unit S = largest fraction-digit count in the group. Value = mantissa · 10^(S − s).
- The spelling's own s is recomputed: s = S − (trailing decimal zeros of the value, at most S),
  and s = 0 for the value 0 ("canonical", what every writer that trims zeros produces), or s = S
  for all values ("fixed", writers with a constant number of decimals).
- A group with an escape-coded number, a spelling that is neither canonical nor fixed, or a
  mantissa that would overflow keeps the current coding. 84% of content-stream values (5.15 M of
  6.13 M) are in groups that convert.
- Format: mode byte bit 6 = unit form, followed by one byte (S, fixed flag). The lifter's
  round-trip check already covers it.

### P2. `Td`/`TD` advance from a learned table

Producers that place words or single glyphs with `Td` write the advance of the text they just
showed. The same text in the same font gives the same advance.

- Key: (font name of the last `Tf`, its size, bytes shown since the last positioning operator).
  Value: the last (x, y) written for that key.
- Known key: store x − x', y − y'. Unknown key: store the values as now. No flag is needed, the
  decoder has the same table.
- Prototype: 491,879 of 587,236 `Td`/`TD` operations hit, 418,202 of those with residual 0.
- The decoder needs what the lifter's grouping pass already has: the skeleton ahead of the
  numbers (operator after its operands) and the strings (skeleton or `.csh`, after P4 the string
  stream). Font size is itself a lifted number, decoded earlier in the same order.
- Depends on P1 (residuals need one unit).

### P3. Path coordinates against the current point

`m`, `l`: x − cx, y − cy. `c`: first control point against the current point, each next point
against the one before. `v`, `y`, `re` alike; `h` returns to the subpath start. Residuals stay in
their (operator, slot) groups, and each group keeps the better of the current coding and the
residual coding (one mode bit). Depends on P1.

### P4. Literal strings out of the `.cs` skeleton

Same shape as `.csh`: a marker byte in the skeleton, a new aux file with a length section and the
string bytes exactly as written (escapes untouched: unescaping measured +3.2%). The skeleton
alone compresses to 550 KB, the strings to 1428 KB.

### P5. Well-known ICC profiles

A stream whose decoded bytes equal a built-in profile is stored as its table index (a layer
with an empty stored form). Start with the two profiles above; add by SHA-256 as corpora show
them.

### P6. Type 1 charstring operands

Pools per PDF, xz: operands 910 KB, skeleton 141 KB, operators 101 KB, cleartext 60 KB, IV 8 KB.
The operand pool is one stream of Type 1 number encodings for all operators. Experiment:
group operands by (operator, slot) as in content streams (hint positions, `rmoveto`, `rlineto`
and `rrcurveto` deltas have different distributions), as integers. Grouping the pools of all
fonts of a PDF together is already measured: 1228 → 1206 KB (−1.8%), so not worth a format change
on its own.

### P7. Item lengths

Channel N is 54 KB of xz, mostly one length per item. An object item ends at `endobj` plus its
line end; a decoder that lexes the text channel finds that point itself. Explicit length only
where the lexer's answer differs. Channel S (item tags, stream descriptors) is another 26 KB.

### P8. Values computable from other objects

- `/Widths` and `/W`: 27.4 KB + 17.1 KB of `.txn`. Needs glyph advances from TrueType `hmtx`,
  CFF charstrings and Type 1 `hsbw`, scaled and rounded the way the producer did.
- Structure tree: `/K` 26.2 KB, `/P` 15.4 KB, `/Nums` 7.7 KB. `/ParentTree` and `/P` are the
  inverse of `/K`.
- Both need objects the decoder has not restored yet at that point, i.e. the two-phase decoder
  the first plan describes for graph predictors.

## 4. Measured and not adopted in this round

| Idea | Result |
|---|---|
| `TJ` kerning from a table per (font, glyph before, glyph after) | numbers +2.5% |
| `TJ` strings and kerning in one stream | numbers + strings +2.7% against today's separate streams, even with P2 and P3 applied to the rest |
| `TJ` numbers grouped per font | −0.7% |
| Literal strings grouped per font | −1.2% |
| Hex strings grouped per font | −0.3% |
| Unescaping content-stream literal strings | +3.2% |
| Hex strings of object text as binary | +1.0% of `.txt` |
| Literal strings of object text as a side stream | −0.8% of `.txt` |
| Raw samples instead of BMP for images under the `-p` threshold | +3.7 KB |
| Own files ordered by extension in the tar | +2 KB |
| XMP text out of `.rec` | −1 KB |

The tar container itself costs 34 KB over 1543 files (22 bytes per file); a single-file dump
mode would save that and nothing else.

## 5. Not in this corpus, cheap to cover when a file shows up

- Base64 in text streams other than XMP thumbnails (XFA forms, SVG or XML attachments with
  `data:` URIs, JavaScript streams): the object-text detector applied to text-class streams.
- Illustrator private data with compressed blocks (`%AI12_CompressedData`).
- Hex-form `eexec` in Type 1 fonts (binary form only is handled).

## 6. Reproducing

    i=0; for f in corpus/*.pdf; do i=$((i+1)); mkdir -p work/s1/$i; ./pdfdump c -q -n -s 1 "$f" work/s1/$i/d; done
    python3 tests/csproto.py work          # P2, P3 and the rejected TJ / per-font variants

P1's figure comes from a build of the lifter that writes both codings of every group and
compresses each; the per-operator and per-key tables come from the same build.
