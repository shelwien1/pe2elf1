# Outside data in ddsdet and xadpcm

*"Outside data" = everything the main model does not model: container headers,
per-record metadata, the bytes between and around the payloads, and payloads the
codec cannot handle at all. Both programs are payload compressors wrapped in a
carver, so both have to answer the same question — what happens to the bytes that
are not payload — and they answer it differently in three places.*

---

## 1. The byte coder

Both reduce to the same thing: an 8-bit binary tree over one order-1 row.

**ddsdet**, `d5c_codec_level.inc`:

```c
void code_tail(u8P d, size_t n, int& prev) {
  scat = 7;
  for(size_t i = 0; i < n; i++) {
    CtrP r = tTail.row(int(MakeTL(u32(prev))));
    int nd = 1;
    for(int k = 7; k >= 0; k--) { ... nd = nd + nd + b; }
    if(!enc) d[i] = u8(nd - 256);
    prev = d[i];
  }
}
```

**xadpcm**, `xad_codec_bits.inc`:

```c
int code_byte(int stream, int prev, int v) {
  scat = ST_LIT;
  CtrP r = tLit.row(stream*LIT_W + prev);
  int nd = 1;
  for(int j = 7; j >= 0; j--) { ... nd = nd + nd + b; }
  return nd - LIT_W;
}
```

Same tree, same shape, same `prev`-carrying overload for chunked streaming — both
grew that overload for the same reason, so a run split across windows still sees
one continuous byte sequence. Two differences:

| | ddsdet | xadpcm |
|---|---|---|
| context | `MakeTL(prev)` — an IDX index, mask `&11111001`, so **6 bits of `prev`**, 64 rows | `stream*256 + prev` — **all 8 bits**, 256 rows per stream |
| streams | **one**, shared by everything | **six**, and the id is part of the context |
| rate schedule | one global | `RS_LIT`, its own per-role schedule |

xadpcm's is a strict superset. There is nothing in `code_tail` to port in.

---

## 2. What each one sends through it

**ddsdet** sends everything: the DDS header (128 or 148 bytes, verbatim through
`code_tail`), the bytes after the payload, the container gaps between textures,
and the solid archive's own metadata records (mode, varint length, crc32).

**xadpcm** splits by role, and the split is the point — the stream id is in the
row index, so the classes never share statistics:

| stream | carries |
|---|---|
| `LIT_HEAD` | a stored RIFF header |
| `LIT_TAIL` | trailing chunks after a wav's data |
| `LIT_MISC` | runt blocks, partial groups, odd bits |
| `LIT_BPRED` | the MS coefficient-set byte, order-1 on the previous one |
| `LIT_GAP` | bytes ahead of the first wav, and (v5) between wavs |
| `LIT_DESC` | the container's own records — `T_SEG`, `Params`, varints |

Measured: a 2 MB incompressible blob costs 2,072,835 bytes on its own and
2,083,160 alongside a wav that costs 10,343 by itself. 2,083,160 − 2,072,835 =
10,325. **The blob does not cost the wav anything** — which is exactly the
pollution ddsdet's source warns about and pays for with its mode-1 escape.

## 3. Two things xadpcm does that ddsdet has no equivalent of

**Canonical header regeneration.** A RIFF header matching the ADPCM-XQ layout is
rebuilt from `Params` at decode time and costs a flag instead of 60–90 literal
bytes (`build_canon_head`, `write_canon`, `pm.canonical`). ddsdet codes every DDS
header byte through `code_tail`; its header is 128 bytes and it pays for all of
them, every texture.

**Header fields as predictor residuals, not literals.** The per-block ADPCM
headers never touch the literal model at all. `code_signed` codes the block's
first sample as a residual against the NLMS prediction, the step index against
the index carried out of the previous block, MS `iDelta` against the running
value *and* against the previous block's — five models (`SM_SAMP`, `SM_INDEX`,
`SM_DELTA`, `SM_SAMP1`, `SM_DELTAH`) with their own magnitude-class and mantissa
rows. That is the difference between "structured metadata that repeats" and
"bytes", and it is worth far more than any order-1 model on the same bytes.

---

## 4. The one thing ddsdet has that xadpcm does not

**The stored-blob escape.** `d5c_solid.inc`, mode 1 — a texture the codec cannot
model goes out **verbatim, outside the coded stream**:

```c
cx.rc.flush();
cx.rc.raw_put(d, n);
cx.rc.init_enc(rcpin_keep());
```

About ten bytes of bracket, and the blob never enters any model. Only the coder's
low/range are reset; every counter, mixer and hash carries on, so the neighbouring
entries compress exactly as before. The source states the reason plainly: such
data "would barely compress while polluting the statistics that the DDS headers
and real tails depend on."

xadpcm has no escape. Everything goes through the model, and on data the model
cannot predict that is not free:

| input | archive | |
|---|---|---|
| 64 KB random | 77,511 | **+18.27%** |
| 256 KB random | 290,282 | **+10.73%** |
| 1 MB random | 1,100,537 | **+4.96%** |
| 2 MB random | 2,072,745 | **+3.64%** |

The curve is the shape of a learning cost, not a coding cost: about 12 KB of
fixed adaptation for the 65,536 counters in one literal stream, amortised over
whatever the blob's length happens to be. At 64 KB it is 18%; a stored blob would
be 0% plus a few bytes of bracket.

This is not hypothetical for xadpcm. `LIT_GAP` exists precisely because a stream
may hold arbitrary non-wav data around the wavs, and the encoder's own help text
says so: *"anything that is not a wav rides along as literal data."* A wav with a
compressed asset appended to it is the ordinary case, not the pathological one.

---

## 5. So: can xadpcm use ddsdet's outside-data handler?

**The byte coder — no, and there is nothing to gain.** `code_byte` already is
`code_tail`, with a wider context, a per-role rate schedule, and six streams
instead of one. Porting it in would be a downgrade.

**The escape hatch — yes, and it is worth doing.** It is a container change, not
a model change, and v5's record structure already has the right shape: a `T_LIT`
record declares its length before its bytes, so a `T_RAW` record differs only in
what happens after the length.

### Sketch

Add `T_RAW = 3`: varint `n`, then `n` bytes outside the model. On the encoder,
`rc.flush()`, raw-write, `init_enc()`; on the decoder the mirror. Counters,
mixers and match state carry across untouched, exactly as ddsdet's do.

**Applies to `LIT_GAP` and `LIT_TAIL` runs only** — never to the payload, the
headers, or the descriptors. Those are the runs whose content is arbitrary by
definition.

**The decision has to be made before coding, because coding cannot be undone.**
ddsdet gets this for free: it knows a texture is unmodellable because
`parse_dds` failed. xadpcm has no such signal for a literal run, so it needs an
estimate — a cheap order-1 entropy count over the window (no model state
involved, so it is deterministic and costs one pass), and `T_RAW` when the
estimate exceeds roughly 7.9 bits/byte. Being conservative is the safe direction:
a run wrongly sent through the model costs what it costs today, while a run
wrongly stored costs whatever it would have compressed by.

**The window is already the natural unit.** The streaming encoder emits a literal
run per 1 MB window, so the decision is per window, and a file that is half audio
and half archive gets the right treatment on both halves.

### What not to port

ddsdet's **single shared tail stream**. Its own source names the cost — headers
and real tails sharing statistics with unmodellable payload — and xadpcm already
avoids it by putting the stream id in the context. Measured above: the blob and
the wav do not interact. That axis is one where the traffic runs the other way.

---

## 6. Summary

| | ddsdet | xadpcm |
|---|---|---|
| order-1 byte coder | `code_tail`, 64 rows | `code_byte`, 256 rows × 6 streams |
| streams | 1, shared | 6, split by role |
| container header | literal bytes through the model | regenerated from `Params` when canonical |
| record metadata | through the model | through the model (`LIT_DESC`) |
| structured fields | none — DDS header is bytes | predictor residuals (`code_signed`) |
| unmodellable data | **stored verbatim, model untouched** | **through the model: +3.6% to +18%** |

One idea worth taking, in one direction. Two worth not taking, in the other.
