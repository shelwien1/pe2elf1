# Accepting what testfiles/refused/ holds

Seven files sit in `testfiles/refused/`, one per way a file can be an Ogg
that oggcomp will not code.  This is the plan for coding them -- starting
with the ID3 tag, as asked, because it is the commonest of the seven in
the world and the one whose mechanism the others then reuse.

The principle does not change.  oggcomp is lossless, so the only question
it may ask of a file is whether every byte can be given back.  Today it
answers no to anything that is not a clean run of Vorbis pages; the
finding of this plan is that with one general mechanism -- bytes the
parser cannot place are coded as bytes -- the answer is yes to every one
of the seven, and the work divides into *accepting* a file (cheap, and
what refused/ is about) and *compressing* what was accepted (a model per
kind of foreign content, later, and only where it pays).

## 1. What each file is, and what stops it

| file | bytes | what it is | refused by | needs |
|---|---|---|---|---|
| `minbitrate-pad.ogg` | 17687 | Vorbis; `oggenc -m 128` on silence, so libvorbis pads the audio packets with zero bits | `vb_packet.inc:448` "1482 unparsed audio bits" | nothing but a relaxed check -- **step 0** |
| `id3-prefix.ogg` | 2805 | a 42-byte ID3v2.4 tag, then a normal stream | `ogg_stream.inc:142` "no Ogg page at 0" | raw runs -- **phase 1** |
| `trailing-junk.ogg` | 2779 | a normal stream, then 16 bytes of not-Ogg | `ogg_stream.inc:142` "no Ogg page at 2763" | raw runs -- **phase 1** |
| `no-eos.ogg` | 2763 | a normal stream whose last page lacks the end-of-stream flag | `ogg_stream.inc:161` "no end-of-stream page" | an explicit link end -- **phase 2** |
| `badcrc.ogg` | 2763 | a normal stream with one byte flipped in the last page | `ogg_stream.inc:143` "bad CRC" | a CRC override -- **phase 2** |
| `opus.ogg` | 47 | one Ogg page carrying an OpusHead | `vb_packet.inc:220` "Ogg Opus is not supported" | raw pages -- **phase 3** |
| `skeleton.ogg` | 2991 | Vorbis with an Ogg Skeleton stream multiplexed beside it (`oggenc -k`) | `vb_packet.inc:435` "audio before setup" | a second stream in a link -- **phase 4** |

Plus two cases `t.sh` makes for itself: an input cut off mid-page, and
an empty file.  Section 5 says what becomes of each.

## 2. Policy

- **A file with no valid page in it is still refused** -- "not an Ogg
  bitstream", as now.  A raw fallback could code anything; that would
  make oggcomp a bad general-purpose compressor and would silence the one
  refusal a user is glad of.  Valid means: parses, and the CRC matches.
- **Anything else is accepted.**  Bytes before the first page, between
  pages and after the last are coded as bytes.  A page whose CRC is wrong
  keeps its stored CRC.  A stream the link ends without closing is ended
  by the file's end.  A page whose payload the Vorbis layer cannot read is
  coded as bytes behind a modelled page header.
- **Accepting is not compressing.**  Every phase below makes a file
  round-trip; the gain on the foreign part is whatever an order-1 byte
  model gives, which for a 42-byte ID3 tag is nothing worth measuring and
  for a JPEG in an APIC frame is nothing at all.  Models for ID3 text
  frames, Skeleton and Opus are section 6, and separate.
- **The format bumps once.**  `OC_VER` goes to 3 in phase 1, and the v3
  grammar (section 3) carries every hook the later phases use, coded as
  its default value until the phase that needs it arrives.  A stream
  written in phase 1 decodes unchanged after phase 4.  The hooks cost a
  few binary symbols per page whose value is almost always the same, i.e.
  a few hundredths of a bit each once the counters have seen a few pages.
- **The stream is the test**, as in REFACTOR.md, with one difference:
  existing corpus files must *decode* to themselves and must not grow by
  more than the hooks account for; their `.oc` will not be byte-identical
  across the version bump, so section 7 measures size instead.

## 3. The v3 stream

Today (v2):

    stream   := link(1) body ... link(0)
    body     := link_begin page page ... page[eos]

Every page is a Vorbis page of one logical stream, the link ends at the
page whose type has bit 2 set, and the stream ends when `link` says 0.

v3:

    stream   := { rawrun link(1) body } rawrun link(0)
    rawrun   := { F_RAWLEN>0 byte... } F_RAWLEN=0
    body     := link_begin { F_PGMORE=1 rawrun page } F_PGMORE=0
    page     := (the fields oc_page codes today) F_CRCBAD [crc32] F_PGRAW
                packets, or if F_PGRAW: payload bytes

- **`rawrun`** is a run of bytes the parser could not place: a sequence
  of chunks of at most `MAXPAY` (65025) bytes each, ended by a zero
  length.  Chunked, so that neither side needs more than one page-sized
  buffer for a tag of any size.  A run sits in exactly one place: before
  the `link` it precedes if the next thing is a new stream or the end of
  the file, otherwise before the page it precedes inside the link.
- **`F_PGMORE`**, coded before each page, says whether the link has
  another page.  Today the decoder infers that from the end-of-stream bit
  of the previous page; making it explicit is what lets a link end at the
  end of the file with no such bit (phase 2).  Its context is that bit,
  so it costs nothing where the bit is honest.
- **`F_CRCBAD`**, coded after the page header fields, says the stored CRC
  is not the one `ogg_page::emit` would compute; if set, the stored
  32-bit value follows (phase 2).
- **`F_PGRAW`** says the page's payload is coded as bytes, not as Vorbis
  packets (phase 3).  The header fields before it are modelled either way.
- The final `link(0)` is preceded by a `rawrun` for whatever follows the
  last page.

The link-level `rawrun` is coded *before* the encoder knows whether a link
follows, which is why it sits before `link` and not after: the encoder
scans forward to the next valid page first, codes the gap, and only then
sees whether that page is a beginning-of-stream page.

Where the symbols live: `F_RAWLEN`, `F_PGMORE`, `F_CRCBAD` and `F_PGRAW`
are new fields of the `aux` family, coded through `tc_aux` like `F_MORE`
and `F_TAIL`.  Raw bytes go through the `hdr` family as a new tag,
`raw.byte`, exactly as comment-header bytes go through `cmt.byte` today
(`vb_packet.inc`, `io::comment`): the model sees the previous byte under
the tag and its run position, which is an order-1 model with a position
context, and is the phase-1 answer.  The CRC, when it has to be stored,
is four `pg.crc` bytes the same way.

## 4. Phases

### Step 0 -- zero padding, no format change

libvorbis under a minimum bitrate pads audio packets with zero bits to
reach it.  oggcomp refuses any packet with eight or more bits after the
last field (`vb_packet.inc:448`), and separately requires them zero
(`:450`).  But the decoder already reproduces zero padding for free: it
writes a packet into a buffer of the length the page declared, zeroed by
`io`'s constructor (`vb_packet.inc:28`), and only the parsed bits are ever
set.  So the count limit is the whole obstacle.

Replace both lines with one rule -- the encoder checks every trailing bit
is zero, however many there are; the decoder checks nothing, as there is
nothing it could have got wrong:

```c
    if(enc)
      while(pos < len * 8)
        FATAL_UNLESS(bget1() == 0, "vorbis: nonzero audio padding");
```

and the same for the header packets at `:279-281`.  **Tested** in a
scratch copy: `minbitrate-pad.ogg` round-trips byte for byte, 17687 →
1605 bytes (it is silence; the padding costs nothing because the decoder
regenerates it from the packet length), and `music-managed-b96.ogg`
codes to the same 84550 bytes as before -- no accepted file's stream
changes, because no new symbol is coded.  Nonzero padding stays refused
until phase 3's raw page catches it.

This is the one change in the plan with no format bump and no dependency
on the rest.  It can land first, on its own commit, with the file moved
from `refused/` to the corpus.

### Phase 1 -- ID3, and any other bytes that are not a page

Covers `id3-prefix.ogg`, `trailing-junk.ogg`, an ID3v1 trailer, a tag on
each stream of a chained file.  This is the phase that changes the format
and defines all of v3; the rest only use what it defines.

1. **`tc_tables.inc`** -- extend the `F_*` enum: `F_RAWLEN, F_PGMORE,
   F_CRCBAD, F_PGRAW` before `F_N`.  `TC_SGN_AUX` is `F_N`, so the aux
   sign table grows by itself (`MOD/tsvcomp-aux_h.inc:77` sizes it by
   it).  Add `STG_RAW` to the stage enum and `"raw"` to `TC_STAGE`, so
   `-v` reports the bytes spent on foreign content on their own line.
2. **`IDX/tsvcomp-aux.idx`** -- the nine `*_fld` patterns are 13 wide,
   one position per field.  A field numbered 13 or more is clamped onto
   position 13 (`MOD/tsvcomp-aux_p.inc:29`: `__min(13, ...)`; the tuning
   form's `mapping::map` clamps the same way), so without this step the
   four new fields would share `F_USED`'s context bucket in every index:
   correct, but blind.  Widen each pattern to 17 and put a `1` at the new
   positions in the indices where the field should have its own row (at
   least `a` and `s`; the tuner can move the rest).  Then `./mk.sh`.
   MOD/ is regenerated, not edited.
3. **`oc_frame.inc`** -- four small coders beside `oc_link`:
   `oc_rawlen(n)` (`tc_aux(F_RAWLEN, ...)`, range 0..MAXPAY),
   `oc_rawbyte(b)` (`oc_hdr("raw.byte", b, 256)` under `STG_RAW`),
   `oc_more(m, eos)` (`tc_auxc(F_PGMORE, eos, ...)`), and `oc_pgflags`
   coding `F_CRCBAD` and `F_PGRAW` -- both always 0 in this phase, and
   the decoder refuses a 1 with a "coded stream:" message until the
   phase that implements it.  `oc_link_begin` resets nothing new.
4. **`oc_model.inc`** -- the facade forwards them: `rawlen`, `rawbyte`,
   `more`, `pgflags`.
5. **`ogg_stream.inc`**, the encoder (`vb_pack`).  Today the loop is:
   want, parse at the window start, refuse if that fails.  New:

       find_page(): from the window start, the offset of the first
         "OggS" at which ogg_page::parse succeeds AND crc_ok holds,
         or avail() if there is none in the window
       gap = that offset
       while gap: code min(gap, MAXPAY) bytes as one raw chunk; skip;
                  want(); re-run find_page()     -- a tag bigger than
                                                     the window is fine
       code F_RAWLEN=0
       if at EOF: link(0); done
       parse the page (it is known to be valid)
       if no link is open: it must be a BOS page -> link(1), link_begin
       else: F_PGMORE=1
       code the page as today, then F_CRCBAD=0, F_PGRAW=0
       after the page: if its EOS bit is set, F_PGMORE=0 at the top
         of the next iteration and the link closes

   `find_page` is a scan for the four capture bytes plus the two checks
   that already exist; a false "OggS" inside a tag -- an embedded image
   can contain anything -- fails one of them and the scan moves on.  The
   window (`IN_WIN`, 32 MB, refilled below `IN_LOW`) is what bounds the
   scan; a page is always shorter than the refill margin, so a page that
   starts inside the window fits in it, as today.
6. **`ogg_stream.inc`**, the decoder (`vb_unpack`), mirrors it:

       for(;;) {
         rawrun: while((n = t.rawlen())) { n bytes via t.rawbyte(); emit }
         if(!t.link(0)) break
         t.link_begin()
         while(t.more()) { rawrun; t.page(q, cont); flags; packets; emit }
       }

   Raw bytes are emitted through the existing `page_writer` in chunks;
   `body` (MAXPAY bytes) is the buffer.
7. **`oggcomp.cpp`** -- `OC_VER = 3`.  A v2 stream is refused with the
   existing "made by oggcomp version 2" message; nothing promised
   otherwise.
8. **`testfiles/gen.sh`** -- `id3-prefix.ogg` and `trailing-junk.ogg`
   leave `refused/` for the corpus (as `id3-prefix-8k.ogg`,
   `trailing-junk-8k.ogg`), and four files join them, all written by the
   python already there, none needing a new tool:
   `id3-text-8k.ogg` (an ID3v2.3 tag with TIT2, TPE1, TALB and a COMM
   frame, a few hundred bytes -- the case the byte model can do something
   with), `id3-apic-8k.ogg` (an APIC frame holding a small PNG -- the
   case it cannot), `id3v1-trailer-8k.ogg` (the 128-byte "TAG" block
   after the last page), and `chained-id3.ogg` (two streams, each behind
   its own tag: junk between links).
9. **Docs** -- `testfiles/README.md` moves the two rows and adds four;
   `README.md`'s "Running it" says what is accepted now.

What this phase does not do: a page with a bad CRC is *skipped over* by
`find_page` and its bytes coded raw, which round-trips but loses that
page's compression, and if the page carried a continued packet the next
page's continuation has no head and is refused as today.  Phase 2 takes
both.

### Phase 2 -- links that end without saying so, and pages that lie about their CRC

Covers `no-eos.ogg`, `badcrc.ogg`, and `t.sh`'s cut-off input.

- **`F_PGMORE=0` without an EOS bit.**  The encoder's one-page lookahead
  (`find_page` after the current page) already decides `more`; the change
  is that a link may now end because the next valid page is a BOS page or
  because the file ends, not only because the EOS bit said so.
  `vb_pack:161`'s refusal goes.  The decoder needs nothing: it already
  ends the link on `more=0`.  The interaction with continued packets: a
  link ending inside a continued packet (`lw.cont` set) is still refused
  -- the bytes are all there, but the packet's head has been coded as
  Vorbis and its tail never arrives, so the head would have to be
  re-coded raw; that is phase 3's raw page applied retroactively, and not
  worth it for a file that is cut off mid-packet.  So: a cut-off input
  whose last page is whole is accepted; one cut inside a page is accepted
  (the fragment is a trailing raw run); one cut between the pages of a
  continued packet is refused.
- **`F_CRCBAD=1`.**  `find_page` accepts a page that parses but whose CRC
  fails *if* it is followed by the next valid page where its length says
  (or by the end of the file) -- that is, when it is a page with a wrong
  checksum rather than junk that happens to start with "OggS".  The
  encoder codes the page normally, sets the flag, and stores the four
  CRC bytes; the decoder writes them over what `emit` computed.  Cost:
  one near-certain symbol per page.

### Phase 3 -- pages that are not Vorbis

Covers `opus.ogg`, and any single-stream file of another codec: FLAC in
Ogg, Speex, Theora-only.  Also nonzero padding in a Vorbis packet, and
any Vorbis packet the parser rejects for a reason that is not a
structural fault of the file (an unsupported floor type, a residue book
without a lookup table -- the "unreachable from oggenc" list in
`testfiles/README.md`).

- A link whose first packet is not a Vorbis identification header is a
  **raw link**: every page of it has `F_PGRAW=1`, header fields modelled
  by `oc_page` as now (they are not codec-specific), payload bytes coded
  under `raw.byte`.  `io::ident`'s Opus refusal goes; `link_walk` learns
  to not hand packets to `io` for such a link.
- A Vorbis link in which one packet cannot be parsed makes that packet's
  **page** raw and the rest of the link continues as Vorbis -- possible
  because the Vorbis state that matters across packets is the setup
  header, which a raw audio page does not change.  A raw page inside a
  Vorbis link still updates `tc_prevW`/`tc_prevmode` to unknown; simplest
  is to reset them as `oc_link_begin` does.
- The encoder needs to know *before* coding a page's packets that one of
  them will be refused.  Today `io` reports by `FATAL`.  The change is a
  dry parse: run the packet through `io` against a `sink` that records
  nothing, and on failure code the page raw.  That doubles the parse cost
  on every packet; alternatively give `io` a soft-fail mode.  Either is a
  contained change to `vb_packet.inc` and `link_walk::page`.

### Phase 4 -- more than one logical stream in a link

Covers `skeleton.ogg`, and with it the ordinary `.ogv` (Theora video
with a Vorbis track) and any Skeleton-wrapped file.

- A link becomes a *group* of logical streams keyed by serial number.  At
  most one of them is Vorbis (the model state -- `vb_su`, the histories,
  `oc_v` -- is one instance); the others are raw streams, their pages
  `F_PGRAW=1`.  A second Vorbis stream in the same group is refused with
  a new message; supporting two would mean instancing the whole model.
- `link_walk` keeps `cont`/`spill` per serial, and `oc_page`'s serial
  context (`F_SERIAL`, delta from the previous page's serial) will see
  the serials alternate; give it the previous page's serial *and* the
  one before as context, so an A-B-A-B pattern costs nothing after the
  first few.
- The group ends when every stream in it has ended (EOS bit, or the
  file's end via `F_PGMORE`).

### After that

`skeleton.ogg` and `opus.ogg` will round-trip with their payloads
uncompressed; the whole of `refused/` will be empty; and what remains is
modelling: ID3 text frames (a dictionary of frame ids and an order-2
model over the text), Skeleton (a handful of fixed-layout packets),
Opus (a real project, out of scope here).  Each is a family of its own
in `IDX/` and a stage in `-v`, and each is judged the same way: bytes
saved on a corpus that has enough of it to matter.

## 5. Tests, per phase

`t.sh` has two kinds of refusal test: files in `testfiles/refused/`, all
expected to exit 1, and eleven cases it builds for itself.  As files are
accepted they leave `refused/` for the corpus, where `t.sh` round-trips
them, checks they code deterministically, and `mk.sh check` proves both
builds agree on them.  `gen.sh` writes them where they now belong, under
a name that says what they are.

| case | today | step 0 | phase 1 | phase 2 | phase 3 | phase 4 |
|---|---|---|---|---|---|---|
| `minbitrate-pad.ogg` | refused | **corpus** | | | | |
| `id3-prefix.ogg`, `trailing-junk.ogg` | refused | | **corpus** | | | |
| `no-eos.ogg`, `badcrc.ogg` | refused | | refused | **corpus** | | |
| `opus.ogg` | refused | | | | **corpus** | |
| `skeleton.ogg` | refused | | | | | **corpus** |
| `t.sh`: cut-off input | exit 1 | | exit 1 ("no end-of-stream") | **round-trips** | | |
| `t.sh`: empty input | exit 1 | unchanged throughout: no valid page | | | | |
| `t.sh`: a `.oc` fed to `c` | exit 1 | unchanged throughout: no valid page | | | | |

Two new negative cases belong in `refused/` once phase 1 lands, because
they are the refusals that survive it and nothing else exercises them: a
file cut between the two pages of a continued packet (from
`bigcomment-8k.ogg`, phase 2's stated limit), and a file of random bytes
with one valid page embedded in the middle -- which is *accepted*, and is
there to prove the scan does not take a false "OggS" for a page.  (That
one goes in the corpus, not `refused/`.)

`t.sh` itself changes in one place: the comment at line 319 and the
expectation for the cut-off case at phase 2.  The loop over `refused/`
needs no change; it tests whatever is there.

## 6. What to expect

| file | today | after |
|---|---|---|
| `minbitrate-pad.ogg` | refused | 1605 bytes (measured) |
| `id3-prefix.ogg` | refused | about `tiny-8k-q0`'s 1354 plus some tens of bytes for a 42-byte tag that is mostly zeros |
| `trailing-junk.ogg` | refused | 1354 plus about 16 |
| `id3-text-8k.ogg` (new) | -- | the tag's text at perhaps 5-6 bits a byte under `raw.byte`; a dedicated model later would halve that |
| `id3-apic-8k.ogg` (new) | -- | the PNG at 8 bits a byte, as it should be |
| `opus.ogg` | refused | header modelled, 19 payload bytes raw |
| `skeleton.ogg` | refused | Vorbis part as today, Skeleton pages raw |
| every existing corpus file | | larger by the v3 hooks: three symbols a page, one a link.  Budget: under 0.1% on `music-stereo-q5.ogg`, and section 7 measures it |

## 7. Verification, each phase

Before the phase, from the unchanged source:

    ./mk.sh && cp oggcomp /tmp/ref
    for f in testfiles/*.ogg; do /tmp/ref c "$f" "/tmp/ref-$(basename "$f").oc"; done

After it:

    ./t.sh && ./mk.sh check                      # the corpus, now including what moved

    #  step 0 only: no accepted file's stream may change
    for f in testfiles/*.ogg; do ./oggcomp c "$f" /tmp/n.oc
      cmp -s /tmp/n.oc "/tmp/ref-$(basename "$f").oc" || echo "CHANGED: $f"; done

    #  phase 1 on: the stream changes (v3); the size may not, by more than the hooks
    for f in testfiles/*.ogg; do ./oggcomp c "$f" /tmp/n.oc
      printf '%-28s %8d -> %8d  %+.3f%%\n' "$(basename "$f")" \
        "$(wc -c < "/tmp/ref-$(basename "$f").oc")" "$(wc -c < /tmp/n.oc)" \
        "$(awk -v a="$(wc -c < "/tmp/ref-$(basename "$f").oc")" -v b="$(wc -c < /tmp/n.oc)" 'BEGIN{print 100*(b-a)/a}')"; done

    #  and the files the phase was for
    for f in <the files that moved>; do ./oggcomp c "$f" /tmp/n.oc && ./oggcomp d /tmp/n.oc /tmp/n.ogg && cmp "$f" /tmp/n.ogg && echo "ok $f $(wc -c < /tmp/n.oc)"; done

    #  both compilers, both MOD/ forms, both Windows backends, as REFACTOR.md section 7

A phase whose size table shows a corpus file growing by more than its
share of the hooks has a modelling regression, not just a format one, and
does not land until that is understood.

## Appendix -- the step-0 patch, as tested

Applied to `vb_packet.inc` in a scratch copy; the audio half is the one
that was run, the header half is the same change at the other site.

```python
p = 'vb_packet.inc'; s = open(p).read()
old = '''    FATAL_UNLESS(len * 8 - pos < 8, "vorbis: %" PRIu64 " unparsed audio bits", (u64)(len * 8 - pos));
    if(enc)
      FATAL_UNLESS(bget((int)(len * 8 - pos)) == 0, "vorbis: nonzero audio padding");'''
new = '''    if(enc)
      while(pos < len * 8)
        FATAL_UNLESS(bget1() == 0, "vorbis: nonzero audio padding");'''
assert s.count(old) == 1; s = s.replace(old, new)
old = '''    FATAL_UNLESS(len * 8 - pos < 8, "vorbis: %" PRIu64 " unparsed packet bits", (u64)(len * 8 - pos));
    if(enc)
      FATAL_UNLESS(bget((int)(len * 8 - pos)) == 0, "vorbis: nonzero packet padding");'''
new = '''    if(enc)
      while(pos < len * 8)
        FATAL_UNLESS(bget1() == 0, "vorbis: nonzero packet padding");'''
assert s.count(old) == 1; s = s.replace(old, new)
open(p, 'w').write(s)
```

Result: `minbitrate-pad.ogg` 17687 → 1605, decodes byte for byte;
`music-managed-b96.ogg` 84550 before and after; `t.sh` fails only on the
line that expected the refusal.
