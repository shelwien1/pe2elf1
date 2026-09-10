# Accepting every input

> **Status.**  Phase 1 is in the tree, and the three things phase 2
> listed first came with it: the v3 stream is what section 3 describes,
> `oggcomp c` cannot exit 1, and every file that was in `refused/` is in
> the corpus and round-trips.  Where the code differs from the plan below:
>
> - A page with a bad CRC is not stored raw.  The page is parsed as it
>   stands and the four CRC bytes it carries are coded after the header
>   (`F_CRCBAD`, then four `raw` symbols); the decoder writes them over
>   what `emit` computed.  `badcrc-8k.ogg` codes to 1362 bytes, 5 more
>   than the undamaged file.
> - Padding is coded per packet, not refused and not assumed zero: after
>   the last field the packet's remaining bits are checked, a flag says
>   whether any is set (`F_PADNZ`), and if so the tail bytes are coded
>   raw.  Zero padding, which is what libvorbis writes, costs the flag.
> - A Vorbis packet that spans a page of another stream is joined across
>   it: the continuation search follows the packet's own serial and steps
>   over the rest.  `skeleton-8k.ogg` codes every Vorbis page as Vorbis.
> - The dry parse unwinds with a C++ exception (`vb_dry_refusal`), not
>   `setjmp`: `Lib3/coro3b.inc` defines its own `jmp_buf` and blocks
>   `<setjmp.h>`, and nothing in the build forbids exceptions.
> - Raw bytes have their own small model, `oc_raw.inc` -- order-0 and
>   order-1 counters mixed by a weight set the bit tree picks -- rather
>   than a tag on the `hdr` family.  Random bytes grow by 0.15% at 1 MB
>   (1.8% at 4 kB, which is the learning cost); text codes at 5-6 bits
>   a byte.  It is what phase 3 would replace.
> - Groups are as section 3 says, and the parse-before-commit is per page:
>   the first page of a group that parses as Vorbis claims the group's
>   Vorbis serial; every later page of that serial is parsed dry and goes
>   raw only if the parser refuses it.
> - The stream ends with Ogg's CRC-32 of the whole input, 32 flat bits,
>   and the decoder refuses to keep what does not check; and it refuses a
>   stream it has read more than 48 bytes past the end of.  Neither was
>   in the plan.  Both are needed once bytes are coded: the Vorbis
>   parser's checks caught a `.oc` cut short or damaged, and a run of
>   bytes has no such checks -- a `.oc` of a non-Ogg file cut in half
>   decoded to something, silently, with exit 0.
>
> Measured on this tree: the corpus files that were already Vorbis grow by
> 6 to 11 bytes each -- four of them the CRC, the rest the link and page
> flags -- 0.024% over the sixteen; `music-stereo-q5.ogg` 144299 to
> 144310, and its encode time is unchanged.  `./t.sh` and `./mk.sh check`
> pass over the 30 files.  Phase 3 -- models for the foreign bytes -- is
> what remains.

Seven files sit in `testfiles/refused/`, one per way a file can be an Ogg
that oggcomp will not code.  This is the plan for coding them -- and for
coding anything else, because the requirement behind it is stronger than
the seven: **`oggcomp c` takes any file and gives it back byte for byte.**
A tool that refuses is a tool the caller has to wrap in a fallback, which
makes it not a tool.  So the plan's first format change makes refusal
impossible on the encode side, and everything after it is about how well
the foreign parts compress.

The mechanism is one thing: bytes the parser cannot place are coded as
bytes.  Around it, three decisions -- what a "link" is when the file does
not say, how the encoder knows a page will not parse before it has spent
any bits on it, and where the fallback bytes go in the model.  ID3 tags,
as asked, are the first thing the mechanism meets, and the test files are
built around them.

## 1. What each file is, and what stops it

| file | bytes | what it is | refused by | after phase 1 |
|---|---|---|---|---|
| `minbitrate-pad.ogg` | 17687 | Vorbis; `oggenc -m 128` on silence, so libvorbis pads the audio packets with zero bits | `vb_packet.inc:448` "1482 unparsed audio bits" | compressed as Vorbis -- and by **step 0** already, with no format change |
| `id3-prefix.ogg` | 2805 | a 42-byte ID3v2.4 tag, then a normal stream | `ogg_stream.inc:142` "no Ogg page at 0" | tag as a raw run, stream as Vorbis |
| `trailing-junk.ogg` | 2779 | a normal stream, then 16 bytes of not-Ogg | `ogg_stream.inc:142` "no Ogg page at 2763" | stream as Vorbis, 16 bytes raw |
| `no-eos.ogg` | 2763 | a normal stream whose last page lacks the end-of-stream flag | `ogg_stream.inc:161` "no end-of-stream page" | compressed as Vorbis; the link just ends |
| `badcrc.ogg` | 2763 | a normal stream with one byte flipped in the last page | `ogg_stream.inc:143` "bad CRC" | that page raw, the rest Vorbis; **phase 2** compresses it too |
| `opus.ogg` | 47 | one Ogg page carrying an OpusHead | `vb_packet.inc:220` "Ogg Opus is not supported" | page header modelled, payload raw |
| `skeleton.ogg` | 2991 | Vorbis with an Ogg Skeleton stream multiplexed beside it (`oggenc -k`) | `vb_packet.inc:435` "audio before setup" | Skeleton pages raw, Vorbis pages Vorbis; **phase 2** keeps Vorbis packets that span a Skeleton page compressed |

And the inputs `t.sh` makes for itself -- an empty file, a file cut off
mid-page, a `.oc` fed to `c` -- all of which round-trip after phase 1.

## 2. The contract

- **`oggcomp c` never refuses.**  Any byte sequence in, the same
  sequence out of `oggcomp d`.  Exit 1 stops being a possible outcome of
  `c`; what remains is 2 (the command line) and 3 (the filesystem).
  `oggcomp d` still refuses what is not an oggcomp stream, which is the
  archive side and is right.
- **What varies is only how much it compresses.**  Vorbis pages compress
  as today.  Everything else -- a tag, a Skeleton page, an Opus payload, a
  page with a bad CRC, a file that is not Ogg at all -- goes through a
  byte model.  On content with structure that is a modest gain; on random
  bytes it is a little over 8 bits a byte, so a file with nothing Ogg in
  it comes out marginally larger than it went in.  If "never larger"
  ever matters, a stored chunk (bytes coded at a flat 1/256) is one more
  aux flag; it is not in this plan because the adaptive model converges
  to within a fraction of a percent of that on its own.
- **The format bumps once.**  `OC_VER` goes to 3 in phase 1, and the v3
  grammar (section 3) carries every hook the later phases use, coded as
  its default until the phase that needs it arrives.  A stream written in
  phase 1 decodes unchanged after phase 3.  The hooks cost a few binary
  symbols per page whose value is almost always the same -- a few
  hundredths of a bit each once the counters have seen a few pages.
- **The stream is the test**, as in REFACTOR.md, with one difference:
  existing corpus files must *decode* to themselves and must not grow by
  more than the hooks account for; their `.oc` will not be byte-identical
  across the version bump, so section 7 measures size instead.

## 3. The v3 stream

Today (v2):

    stream := link(1) body ... link(0)
    body   := link_begin page page ... page[eos]

Every page is a Vorbis page of one logical stream; the link ends at the
page whose type has the end-of-stream bit; the stream ends when `link`
says 0; anything else is a refusal.

v3:

    stream := { rawrun link(1) body } rawrun link(0)
    rawrun := { F_RAWLEN>0 byte... } F_RAWLEN=0
    body   := link_begin { F_PGMORE=1 rawrun page } F_PGMORE=0
    page   := (the fields oc_page codes today)  F_PGRAW  [F_CRCBAD crc32]
              then: packets as today, or if F_PGRAW the payload as bytes

- **A link is structural, not semantic.**  A beginning-of-stream page
  starts one; it runs to the next beginning-of-stream page or the end of
  the file.  The end-of-stream bit is coded as part of the page type and
  decides nothing.  That is what makes a link that never says it ended,
  a file cut off anywhere, and a multiplexed file all fit the same
  grammar: `skeleton.ogg` is two links, the second holding the Vorbis
  stream with Skeleton pages interleaved among its own.
- **`rawrun`** is bytes the parser could not place: a sequence of chunks
  of at most `MAXPAY` (65025) bytes, ended by a zero length.  Chunked, so
  neither side needs a buffer larger than a page for a tag of any size.
  A run sits in exactly one place: before the `link` it precedes when the
  next thing is a beginning-of-stream page or the end of the file, else
  before the page it precedes inside the link.  A file with no page in it
  is one `rawrun` and `link(0)`; an empty file is `F_RAWLEN=0 link(0)`.
- **`F_PGMORE`**, before each page, says the link has another.  Its
  context is the previous page's end-of-stream bit, so where the bit is
  honest it costs nothing.
- **`F_PGRAW`** says the page's payload is coded as bytes.  The header
  fields before it are modelled either way -- they are Ogg, not Vorbis,
  and `oc_page` does not care what the packets hold.
- **`F_CRCBAD`** says the stored CRC is not the one `ogg_page::emit`
  would compute; if set, the stored value follows.  Coded 0 in phase 1
  (a page with a bad CRC is not a page to phase 1's scan; it is part of a
  run) and used in phase 2.
- The final `link(0)` is preceded by a `rawrun` for whatever follows the
  last page.

The link-level `rawrun` sits *before* `link` rather than after it because
the encoder cannot know a link is starting until it has found the next
valid page, which is after the gap.  It scans, codes the gap, then looks.

Where the symbols live: `F_RAWLEN`, `F_PGMORE`, `F_PGRAW` and `F_CRCBAD`
are new fields of the `aux` family, coded through `tc_aux` like `F_MORE`
and `F_TAIL`.  Raw bytes go through the `hdr` family as a new tag,
`raw.byte`, exactly as comment-header bytes go through `cmt.byte` today
(`io::comment`): the model sees the previous byte under the tag and the
run position, an order-1 model with a position context.  The stored CRC,
when there is one, is four `pg.crc` bytes the same way.

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
changes, because no new symbol is coded.  Nonzero padding -- which no
encoder writes -- is not this step's; phase 1's raw page takes it.

This is the one change in the plan with no format bump and no dependency
on the rest.  It can land first, on its own commit, with the file moved
from `refused/` to the corpus.

### Phase 1 -- everything round-trips

This is the phase that changes the format and defines all of v3, and
the phase after which `oggcomp c` cannot exit 1.  It has three parts:
the run (junk between pages), the structure (links delimited by
beginning-of-stream pages), and the raw page (a page the Vorbis layer
would refuse).  The third is most of the work.

**Symbols and model.**

1. `tc_tables.inc` -- extend the `F_*` enum: `F_RAWLEN, F_PGMORE,
   F_PGRAW, F_CRCBAD` before `F_N`.  `TC_SGN_AUX` is `F_N`, so the aux
   sign table grows by itself (`MOD/tsvcomp-aux_h.inc:77` sizes it by
   it).  Add `STG_RAW` to the stage enum and `"raw"` to `TC_STAGE`, so
   `-v` shows what the foreign bytes cost on a line of their own.
2. `IDX/tsvcomp-aux.idx` -- the nine `*_fld` patterns are 13 wide, one
   position per field.  A field numbered 13 or more is clamped onto
   position 13 (`MOD/tsvcomp-aux_p.inc:29`: `__min(13, ...)`; the tuning
   form's `mapping::map` clamps the same way), so without this step the
   new fields share `F_USED`'s context in every index: correct, but
   blind.  Widen each pattern to 17 with a `1` at the new positions in
   the indices where the field should have its own row (at least `a`
   and `s`; the tuner can move the rest).  Then `./mk.sh`.  `MOD/` is
   regenerated, never edited.
3. `oc_frame.inc` -- beside `oc_link`: `oc_rawlen(n)` (`tc_aux(F_RAWLEN,
   ...)`, 0..MAXPAY), `oc_rawbyte(b)` (`oc_hdr("raw.byte", b, 256)`
   under `STG_RAW`), `oc_more(m, eos)` (`tc_auxc(F_PGMORE, eos, ...)`),
   `oc_pgraw(r)` and `oc_crcbad(c)`.  The decoder refuses a
   `F_CRCBAD=1` with a "coded stream:" message until phase 2 -- it can
   only come from a hand-made stream.
4. `oc_model.inc` -- the facade forwards `rawlen`, `rawbyte`, `more`,
   `pgraw`, `crcbad`.  And a second implementation of the same
   interface, `null_sink`, whose every method returns its input and
   touches nothing -- this is what the dry parse below runs against.

**The encoder, `vb_pack` in `ogg_stream.inc`.**  Today: want, parse at
the window start, refuse if that fails.  New:

    find_page(): from the window start, the offset of the first "OggS"
      at which ogg_page::parse succeeds AND crc_ok holds -- or avail()
      if there is none in the window
    gap = that offset
    while gap: code min(gap, MAXPAY) bytes as a raw chunk; skip; want();
               find_page() again           -- a tag bigger than the
                                              window is just more chunks
    code F_RAWLEN=0
    at EOF: link(0); done
    parse the page; it is known to be valid
    if no link is open, or the page is a beginning-of-stream page:
      close the open link (F_PGMORE=0) if any; link(1); link_begin()
    else F_PGMORE=1
    oc_page as today
    raw = !page_fits(p)                  -- see below
    F_PGRAW=raw
    if raw: payload bytes via rawbyte, cont=0, header count unchanged,
            tc_prevW/tc_prevmode reset as oc_link_begin does
    else:   packets as today

`find_page` is a scan for the four capture bytes plus the two checks
that exist today.  A false "OggS" inside a tag -- an embedded image can
contain anything -- fails one of them and the scan moves on.  The window
(`IN_WIN`, 32 MB, refilled below `IN_LOW`) bounds the scan; a page is
always shorter than the refill margin, so a page that starts inside the
window fits in it, as today.

**`page_fits`, the dry run.**  The encoder has to know that a page's
packets will parse *before* it codes them, because once `io` has coded a
packet through the real model there is no taking it back -- the model
state is a gigabyte and cannot be checkpointed.  So every page is parsed
twice: once against `null_sink` to learn whether it parses, once against
the real model to code it.  The parser is deterministic and its verdict
depends only on the bytes and the current setup header, not on model
state, so the two runs agree.

- `struct io` becomes `template <class S> struct io_t`, with
  `typedef io_t<oc_model> io`.  Its methods already call the sink only
  through `t.`; nothing else changes.
- Its 48 refusal sites, the 4 in `link_walk::page` and the 5 in
  `source::join_len`/`join_pkt` route through one function that in a
  live parse calls `FATAL` as now and in a dry parse unwinds to
  `page_fits`.  `setjmp`/`longjmp` will do: `io` has no destructors, and
  nothing in `io` or `source` yields (checked -- the only path to the
  coroutine is through the real sink, which the dry run never reaches).
  C++ exceptions would also do, and nothing in the build forbids them,
  but the tree does not use them and `Lib3/` is `setjmp` already.
- A dry parse of a *setup* header writes the parsed codebooks somewhere.
  Give it a scratch `vb_setup` (331 KB, static) and restore `vb_used`
  afterwards; the real parse then does the work again on the real one.
  Once per link, so it does not matter.
- Cost: every packet is parsed twice.  Parsing is a small part of
  encoding -- the model is what the time goes on -- so the budget is
  5%, measured on `music-stereo-q5.ogg` before this lands.  If it is
  more, the dry parse can skip pages that are obviously fine (a page
  with no continuation, in a link whose setup has parsed, whose packets
  all start with an audio byte) and dry-run only the doubtful ones.

`page_fits` also covers what `vb_pack` refuses today outside `io`: a
continued packet whose continuation is missing or cut off, a fourth
header packet, a page after the link's setup that is not audio.  All of
those make the page raw and the file goes on.  A raw page that carried
the head of a continued packet leaves `cont` clear, so the next page's
continuation arrives as a first packet that does not parse, and that
page goes raw too -- the fallback is self-consistent without either side
tracking why.

**The decoder, `vb_unpack`.**  Mirrors it:

    for(;;) {
      rawrun: while((n = t.rawlen())) { n bytes via t.rawbyte(); emit }
      if(!t.link(0)) break
      t.link_begin()
      while(t.more()) {
        rawrun; t.page(q, cont); raw = t.pgraw()
        if raw: payload bytes via t.rawbyte() into body; cont = 0
        else:   packets as today
        emit the page
      }
    }

The decoder never decides anything; it follows the flags.  Raw bytes go
out through the existing `page_writer` in chunks, with `body` (MAXPAY
bytes) as the buffer.

**The rest.**

5. `oggcomp.cpp` -- `OC_VER = 3`.  A v2 stream is refused by `d` with
   the existing "made by oggcomp version 2" message.
6. `testfiles/gen.sh` -- everything in `refused/` moves to the corpus,
   under names that say what each is (`id3-prefix-8k.ogg`,
   `trailing-junk-8k.ogg`, `no-eos-8k.ogg`, `badcrc-8k.ogg`, `opus.ogg`,
   `skeleton-8k.ogg`), and `refused/` stops being made.  Seven files
   join, all written by the python already there: `id3-text-8k.ogg` (an
   ID3v2.3 tag with TIT2, TPE1, TALB and a COMM frame -- text the byte
   model can do something with), `id3-apic-8k.ogg` (an APIC frame
   holding a small PNG -- bytes it cannot), `id3v1-trailer-8k.ogg` (the
   128-byte "TAG" block after the last page), `chained-id3.ogg` (two
   streams each behind its own tag: junk between links), `empty.ogg`
   (zero bytes), `random.bin` (4 kB from the seeded generator: not Ogg
   at all), and `page-in-random.bin` (one valid page inside random
   bytes: the scan must find it and must not take a false "OggS" for a
   page).
7. `t.sh` -- its three home-made inputs (empty, cut off, a `.oc` fed to
   `c`) move from the refusal section to a round-trip check; the
   refusal section keeps the `.oc`-side and command-line cases, which
   are the ones that remain.  A new section feeds `c` every file in the
   tree that is not an `.ogg` -- the scripts, the READMEs, the `.oc`
   files it has just made -- and requires exit 0 and a byte-exact round
   trip from each.  That section is the contract of section 2 as a
   test.
8. `README.md` -- "Running it": exit 1 is now only `d`'s; and a sentence
   saying what happens to a file that is not Ogg.  `testfiles/README.md`
   loses its `refused/` section and gains the rows.

**What phase 1 stores raw that could be compressed** -- not refusals,
costs: a page with a bad CRC (it is in a run, not a page); the Vorbis
packets on either side of a Skeleton page in a multiplexed file when a
packet spans the two (both pages go raw); non-Vorbis payloads, always.
Phase 2 takes the first two.

### Phase 2 -- compress what phase 1 stored raw

- **`F_CRCBAD=1`.**  `find_page` accepts a page that parses but whose
  CRC fails *when* the next valid page starts where its length says it
  should (or the file ends there) -- a page with a wrong checksum, as
  opposed to junk that happens to begin with "OggS".  The encoder codes
  it as a page, sets the flag and stores the four CRC bytes; the decoder
  writes them over what `emit` computed.  `badcrc.ogg` compresses.
- **Continuation across foreign pages.**  `link_walk` keeps `cont` and
  `spill` per serial number rather than per link, and `source::join_len`
  follows the continuation to the next page *of the same serial* rather
  than the next page.  A Vorbis packet that spans a Skeleton page is then
  joined as it would be without the Skeleton page, and neither page goes
  raw.  `oc_page`'s serial context (`F_SERIAL`, coded as a delta from the
  previous page's serial) sees serials alternate; give it the serial two
  pages back as well, and an A-B-A-B pattern costs nothing after the
  first few.  `skeleton.ogg`, and with it the ordinary `.ogv`, then
  compresses everything Vorbis in it.
- At most one Vorbis stream per link.  The model -- `vb_su`, the
  histories, `oc_v` -- is one instance; a second Vorbis stream in the
  same link goes raw, page by page, through the fallback.  Instancing the
  model for two is a different project.

### Phase 3 -- models for what is foreign

Each of these is a family of its own in `IDX/`, a stage in `-v`, and is
judged the same way: bytes saved on a corpus that has enough of it to
matter.  None changes the format: a `rawrun` or a raw page is still what
it is, and the model behind `raw.byte` is what gets smarter.

- **ID3.**  A tag is a 10-byte header, frames of (id, size, flags,
  payload), padding.  The frame ids are a small dictionary; text frames
  are text; APIC is an image and stays at 8 bits a byte.  An order-2
  model over text with the frame id as context is the whole of it.
- **Skeleton.**  Three fixed-layout packet kinds (fishead, fisbone, the
  index).  Fields, coded like Vorbis header fields through `hdr` tags.
- **Opus.**  A real project, out of scope; the page headers are already
  modelled and that is what phase 1 gives it.

## 5. Tests, per phase

| case | today | step 0 | phase 1 | phase 2 |
|---|---|---|---|---|
| `minbitrate-pad.ogg` | refused | **corpus** | | |
| the other six in `refused/` | refused | | **corpus** | |
| `t.sh`: empty input, cut-off input, a `.oc` fed to `c` | exit 1 | | **round-trip** | |
| `t.sh`: "anything round-trips" over the tree's non-`.ogg` files | -- | | **new** | |
| `t.sh`: `.oc`-side and command-line refusals | exit 1/2/3 | unchanged throughout | | |
| `badcrc-8k.ogg`, `skeleton-8k.ogg` size | -- | | stored | **smaller** |

`t.sh`'s loop over `refused/` tests whatever is in the directory and
needs no change; once `gen.sh` stops making the directory the loop finds
nothing, and the comment above it (line 319) is rewritten to say what
the section now covers.

## 6. What to expect

| input | today | after phase 1 |
|---|---|---|
| `minbitrate-pad.ogg` | refused | 1605 bytes (measured, step 0) |
| `id3-prefix.ogg` | refused | about `tiny-8k-q0`'s 1354 plus some tens of bytes for a 42-byte tag that is mostly zeros |
| `trailing-junk.ogg` | refused | 1354 plus about 16 |
| `no-eos.ogg` | refused | about 1354 |
| `badcrc.ogg` | refused | about 1354 less that page's share, plus that page's 2640 bytes raw; phase 2 brings it to about 1354 |
| `opus.ogg` | refused | a modelled header and 19 payload bytes |
| `skeleton.ogg` | refused | Vorbis pages as today, Skeleton pages raw, and the two pages around any spanning packet raw; phase 2 fixes that |
| `id3-text-8k.ogg` (new) | -- | the tag's text at perhaps 5-6 bits a byte under `raw.byte`; phase 3 halves it |
| `id3-apic-8k.ogg` (new) | -- | the PNG at 8 bits a byte, as it should be |
| `empty.ogg` (new) | -- | the 7-byte header and the coder's flush: under 20 bytes |
| `random.bin` (new) | -- | 4096 bytes in, about 4130 out |
| every existing corpus file | | larger by the v3 hooks: three symbols a page, one a link.  Budget: under 0.1% on `music-stereo-q5.ogg`; section 7 measures it |
| encode time | | the dry parse: budget 5%, measured |

## 7. Verification, each phase

Before the phase, from the unchanged source:

    ./mk.sh && cp oggcomp /tmp/ref
    for f in testfiles/*.ogg; do /tmp/ref c "$f" "/tmp/ref-$(basename "$f").oc"; done
    time /tmp/ref c testfiles/music-stereo-q5.ogg /tmp/ref-time.oc

After it:

    ./t.sh && ./mk.sh check                      # the corpus, now including what moved

    #  step 0 only: no accepted file's stream may change
    for f in testfiles/*.ogg; do ./oggcomp c "$f" /tmp/n.oc
      cmp -s /tmp/n.oc "/tmp/ref-$(basename "$f").oc" || echo "CHANGED: $f"; done

    #  phase 1 on: the stream changes (v3); the size may not, by more than the hooks
    for f in testfiles/*.ogg; do ./oggcomp c "$f" /tmp/n.oc
      a=$(wc -c < "/tmp/ref-$(basename "$f").oc"); b=$(wc -c < /tmp/n.oc)
      printf '%-28s %8d -> %8d  %+.3f%%\n' "$(basename "$f")" "$a" "$b" "$(awk -v a=$a -v b=$b 'BEGIN{print 100*(b-a)/a}')"
    done
    time ./oggcomp c testfiles/music-stereo-q5.ogg /tmp/n.oc      # against /tmp/ref-time

    #  phase 1 on: the contract -- nothing is refused, everything comes back
    for f in testfiles/* testfiles/refused/* *.inc *.sh *.md /tmp/n.oc; do
      [ -f "$f" ] || continue
      ./oggcomp c "$f" /tmp/x.oc && ./oggcomp d /tmp/x.oc /tmp/x.out && cmp -s "$f" /tmp/x.out \
        || echo "NOT ROUND-TRIPPED: $f"
    done

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
