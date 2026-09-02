# The transformer in coder0

What the 6M-parameter model does with the bytes it has seen, layer by layer:
the shapes, the context each part draws on, and why each part is there. The
"why" comes from two places — the ablations the authors report in their
writeup, and the values the trained parameters actually settled on, which are
quoted where they say something the architecture diagram does not.

Sources: `cpp_infer/SPEC.md`, `cpp_infer/KIMI_SEMANTICS.md` and `writeup.md`
of the fx2-cmix-transformer submission; the forward pass as ported in
`tf/fp32_model.inc`; the trained values read out of `models/6m-q4-fp32.tfwc2`.

## 1. In one paragraph

A byte-level autoregressive transformer: vocabulary 205, width 192, twelve
blocks, 5,897,145 parameters. It predicts a distribution over the next byte
from two inputs — the byte just coded, and PPMD's own distribution over the
next byte — plus what it remembers of the article so far. Nine of the twelve
blocks remember through a small recurrent state (Kimi linear attention: a
64×64 matrix per head that decays and is updated every token); three remember
through ordinary attention over the last 1024 tokens. Every block ends with an
MLP. The output layer produces logits that are soft-capped and turned into
probabilities. On enwik9 it reaches 0.911 nats/byte where the LSTM it replaced
reached 1.23.

## 2. What the model is given

| input | shape | what it is |
|---|---|---|
| token | 1 of 205 | the byte just coded, mapped to the model's token id (`transformer.inc` derives the mapping) |
| prior | 205 floats, sums to 1 | PPMD's distribution over the **next** byte — the byte being predicted, not the one just seen |
| position | integer | index within the current context piece; only the three attention layers use it, through RoPE |

The prior is the unusual one. The model is not trained to predict from
scratch; it is trained to *refine* PPMD's guess, which is why its loss can be
so far below a stand-alone byte model of this size. PPMD contributes the
order-9 statistics of everything seen so far; the transformer contributes what
PPMD cannot express — longer-range and softer structure — and learns when to
trust which.

Everything else the prediction depends on is state the model carries from
token to token (§8). Contexts are independent: at the start of every piece the
recurrent states, the convolution histories and the attention caches are
zeroed, so no information crosses a piece boundary. coder0 cuts its input into
pieces of at most 131072 bytes (the length of the RoPE table); the original
cmix cuts at Wikipedia article boundaries.

## 3. Parameter budget

| component | shape | count | × | total |
|---|---|---|---|---|
| token embedding | 205×192 | 39,360 | 1 | 39,360 |
| prior embedding | 192×205 | 39,360 | 1 | 39,360 |
| unembedding | 205×192 | 39,360 | 1 | 39,360 |
| KDA block (attention part) | see §6 | 199,747 | 9 | 1,797,723 |
| attention block (attention part) | 4 × 192×192 | 147,456 | 3 | 442,368 |
| MLP | 768×192 + 192×768 | 294,912 | 12 | 3,538,944 |
| residual / token / skip coefficients | scalars | 30 | 1 | 30 |
| | | | | **5,897,145** |

5,868,864 of these are int4 in the shipped file (every matrix); the remaining
28,281 — convolution taps, `beta` projections, `dt_bias`, the norm gains, the
decay rates and the coefficients — are fp32. The MLPs are 60% of the model.

Per token the forward pass is about 5.8 million multiply-adds, plus the
attention and recurrence work that depends on context length.

## 4. Embedding: building the first residual vector

```
tok = rms_norm( E[token] )            E: 205×192
pri = rms_norm( P · prior )           P: 192×205
x0  = tok + pri
```

Both halves are normalized to unit RMS before being added, so neither can
drown the other: the embedding of the byte and the embedding of PPMD's opinion
enter with equal weight. `P` is a linear map from a probability vector to the
residual width — each token id has a 192-dim column, and the prior mixes those
columns in proportion to the probability PPMD gives each byte. A confident
prior therefore lands near the column of the byte it favours; a flat one lands
in the middle.

`rms_norm(x) = x / sqrt(mean(x²) + ε)` throughout, ε = FLT_EPSILON, no learned
gain except where stated. It is the model's only normalization.

## 5. Block wiring

Each block `l = 0..11` does, in order:

```
if l ≥ 6:   x += skip_w[l−6] · x_out[11−l]            skip connection
x  = rsc[l]·x + tec[l]·tok                            learned rescale, token re-injection
x += attention_l( rms_norm(x) )                       §6 or §7
x += mlp_l( rms_norm(x) )                              §8
if l < 6:   x_out[l] = x                              saved as a skip source
```

**Skip connections** pair the two halves of the stack: block 6 receives the
output of block 5, 7 of 4, … 11 of 0. Trained weights: 0.40, 0.38, 0.61,
−0.18, 0.17, **0.81**. So they are used, and the strongest is the outermost
pair — block 11 gets 81% of block 0's output added straight in, a direct route
from the earliest representation to the last block.

**Residual coefficients** `rsc` scale the stream before each block:
0.004, 0.24, 0.42, 0.69, 0.33, 0.36, 0.29, 0.45, 0.63, 0.74, 0.71, 0.80. The
first is nearly zero. That does not starve block 0 — its input goes through
`rms_norm`, which is scale-invariant — but it does mean the raw embedding sum
is all but dropped from the residual stream, and what blocks 1..11 see as "the
stream" is essentially what block 0 *computed*. The coefficient then grows
with depth: the deeper the block, the more of the accumulated stream it keeps.

**Token-embedding connections** `tec` re-inject the normed token embedding at
every block. Trained values: all between −0.004 and 0.026. The model did not
use them; the authors say they included these Modded-NanoGPT features without
checking whether they help, and for this one the weights agree.

## 6. Kimi linear attention (blocks 0,1,2, 4,5,6, 8,9,10)

Nine of the twelve blocks. A linear-attention layer: instead of attending over
stored keys it keeps a fixed-size state per head that is updated once per token
and read once per token, so the cost per token is constant however long the
context is.

Per block, three heads of 64, on `xn = rms_norm(x)` (192):

```
q, k, v      = Wq·xn, Wk·xn, Wv·xn                      three 192×192
q, k, v      = silu( conv4(q) ), silu( conv4(k) ), silu( conv4(v) )
g_raw        = W_fd · (W_fu · xn)                       192×64 · 64×192  (forget gate, rank 64)
og           = W_od · (W_ou · xn)                       192×64 · 64×192  (output gate, rank 64)
beta[h]      = sigmoid( w_beta[h] · xn )                3 × 192, fp32
```

`conv4` is a causal convolution of width 4 over the token axis, per channel:
each channel of `q`, `k`, `v` is a weighted sum of its value at this token and
the three before it. This is the only place the model looks at a short window
directly, and it is what gives every KDA head a notion of "the last few bytes"
independent of the recurrent state.

Then per head, with state `S[h]` a 64×64 fp32 matrix (rows indexed by the key
dimension, columns by the value dimension), zero at the start of a context:

```
qn, kn   = l2norm(q[h]), l2norm(k[h])
decay[i] = exp( −A[h] · softplus( g_raw[h][i] + dt_bias[h][i] ) )    ∈ (0, 1]
S        = diag(decay) · S                     forget, per key channel
r        = Sᵀ kn                               what the state predicts for this key
S       += kn ⊗ ( beta[h] · (v[h] − r) )        write only the surprise
o[h]     = Sᵀ ( 0.125 · qn )                   read
out[h]   = rms_norm(o[h]) · gn_w · sigmoid(og[h])
y        = Wo · concat(out)                     192×192
```

The update is the **delta rule**: the state is corrected by the difference
between the value it would have produced for this key and the actual value,
scaled by `beta`. Writing `v − r` rather than `v` is what lets a bounded state
keep being useful — repeated keys do not pile up, they converge — and `beta`,
a per-token scalar from the residual stream, lets the model decide how hard to
write.

The **decay** is where the memory horizon lives, and it is per channel and per
token: `g_raw` is a function of the current input, so the model chooses at
every step how much of each key channel's history to keep. The trained
constants set the baseline. `A = exp(log_decay)` ranges from 0.46 to 4.01
across the 27 heads; `dt_bias` averages about −1.4 with a spread from −6.3 to
+1.0. With the gate input near its bias, `softplus(−1.4) ≈ 0.22`, so a typical
head decays by `exp(−0.22·A)` per token — about 0.71 for A = 1.57 (a horizon
of a few bytes), about 0.90 for A = 0.46 (tens of bytes). Channels whose bias
sits at −6 have `softplus ≈ 0.002` and a decay of ≈ 1: they hold what they are
given until the gate says otherwise. A strongly positive gate input drives the
decay to 0 and clears the channel. So the same layer contains channels that
forget within a word, channels that remember across a sentence, and channels
that hold indefinitely, and the input steers between them.

The **output gate** `sigmoid(og)` multiplies the normed read-out per channel —
a second data-dependent decision about what to pass on. `gn_w` (64, shared
across heads) is the one learned gain in the model; its mean rises from 0.25
in block 0 to about 1.5 in blocks 8–10, so the KDA path contributes more
strongly deeper in the stack.

**Why these blocks are here.** The authors tested attention-only against
mixes with 20, 33, 50, 75, 80 and 100% KDA; 75% gave the lowest loss, and it
is also much cheaper per token than attention over a window. KDA's memory is
compressive — 64×64 numbers per head, whatever the context length — which is
a limitation for exact recall but an advantage for the kind of soft statistical
context that a byte model needs most of the time, and it never has to decide
what to drop from a cache.

Per block: 4 × 36,864 (projections) + 2 × 24,576 (the two rank-64 gates) +
2,304 (conv taps) + 576 (beta) + 192 (dt_bias) + 64 (gain) + 3 (decay rates)
= 199,747.

## 7. Sliding-window attention (blocks 3, 7, 11)

Three blocks of ordinary causal attention, restricted to the last 1024 tokens.
Three heads of 64, no grouping:

```
q, k, v  = Wq·xn, Wk·xn, Wv·xn                          three 192×192
q, k     = rms_norm per head (64)                       query-key normalization
q, k     = RoPE(q, pos), RoPE(k, pos)                   base 10000, 32 frequencies
score[j] = 0.125 · q[h] · k_j[h]        for j in the last 1024 tokens (incl. this one)
out[h]   = Σ_j softmax(score)[j] · v_j[h]
y        = Wo · concat(out)                             192×192
```

The keys and values of the last 1024 tokens are kept in a ring per block (in
fp32 here; the packed engine keeps them int8). This is the model's exact
memory: any of the last 1024 bytes can be retrieved by content, which is what
KDA cannot do — a byte model needs both "what usually follows this" and "what
followed this the last time it appeared 800 bytes ago", and the two mechanisms
divide that work.

RoPE gives the dot product a dependence on the distance `pos_q − pos_k`, so a
head can prefer nearby or far keys as well as matching content. Query-key
normalization keeps the scores in a range where the softmax neither saturates
nor flattens, which matters for training stability at this width.

**Why 1024.** The authors tried 512 (higher loss) and 2048 (too slow for the
Hutter Prize time limit). Attention cost grows with the window, KDA's does not,
which is the other half of the reason only a quarter of the blocks use it.

Per block: 4 × 36,864 = 147,456.

## 8. MLP (every block)

```
h = W_up · rms_norm(x)        768×192
h = relu(h)²
y = W_down · h                192×768
```

A 4× expansion with the squared-ReLU nonlinearity, which is sharper than GELU
and produces a sparse hidden layer. Measured on book1: of the 768 units, the
fraction that is exactly zero for a given byte is 48% in block 0 and 70–89% in
every other block (73, 89, 70, 75, 74, 88, 79, 79, 85, 86, 74), the same on the
first 4 KB and the first 16 KB. The packed engine exploits that with a sparse
kernel for `W_down`. This is where the model stores what it knows independent
of context: the MLPs are 60% of the parameters, and they are the part of a
transformer that acts as a lookup from "the current representation" to "what
tends to come next".

Per block: 2 × 147,456 = 294,912.

## 9. Head

```
xn     = rms_norm(x)
logits = U · xn                                  205×192
logits = 15 · tanh(logits / 15)                  soft cap
p      = softmax(logits)
```

The soft cap bounds every logit to (−15, 15), so no byte can be assigned a
probability below about e⁻³⁰ relative to the top one. For a compressor that
is a safety property as much as a training aid: a model that is certain and
wrong costs the arithmetic coder dearly, and the cap limits how certain it can
ever be. coder0 then restricts this distribution to the bytes present in the
file and renormalizes.

## 10. What context reaches a prediction

| mechanism | what it holds | reach | how it is used |
|---|---|---|---|
| the token input | the byte just coded | 1 | embedded directly |
| the prior input | PPMD's distribution over the next byte | PPMD's own (order 9 over the whole file so far) | embedded and added to the token embedding |
| convolutions, 9 blocks | the last 3 tokens' q/k/v projections, per channel | 4 tokens | short-range features for every KDA head |
| KDA state, 9 blocks × 3 heads | one 64×64 matrix each: a decaying, delta-rule-updated key→value map | unbounded, data-dependent — from a few bytes to the whole context per channel | associative recall of soft statistics |
| KV rings, 3 blocks | keys and values of the last 1024 tokens | exactly 1024 tokens | exact content-based retrieval, position-aware via RoPE |
| skip stores | the outputs of blocks 0–5 for **this** token | within one token | routes early representations to late blocks |
| RoPE position | index within the context piece | — | lets attention prefer near or far keys |

So a prediction sees: the current byte, everything PPMD knows, the last four
bytes sharply, the last 1024 bytes exactly (three times over, at different
depths), and a compressed, selectively-forgotten summary of the whole piece
(nine times over). Nothing crosses a piece boundary.

## 11. Quantization, and one consequence for the prior

The shipped file stores every matrix as int4 in [−7, 7] with one bf16 scale
per output row, and every matrix input as int8 with one static scale per
site, both learned with quantization-aware training. The quantized model's
loss is 2.6% above the bfloat16 model's; the authors found this the best
trade of file size against loss, and the −7..7 range (rather than −8..7) costs
nothing because the file entropy-codes each weight in 3.81 bits anyway.

One number worth knowing: the prior's activation scale is 0.00276, and int8
saturates at 127, so in the packed engine any prior probability above about
0.35 is clipped to the same value. The model was trained that way — beyond
"PPMD gives this byte more than a third" it does not distinguish degrees of
confidence, it only sees the shape of the rest of the distribution. The fp32
engine (`TF_FP32=1`, the default here) has no activation quantizers and does
not clip; the measured effect on compression is within a byte or two either
way.

## 12. How coder0 drives it

* **Alphabet.** The model's 205 tokens are the byte values of cmix's
  WRT-preprocessed enwik9, in ascending order. coder0 reconstructs that
  mapping (documented at the top of `transformer.inc`) so that letters,
  digits, punctuation and newline reach the tokens they were trained as;
  upper-case letters, which the preprocessed stream never contains, get
  unclaimed tokens. Feeding the alphabet's own indices instead points every
  embedding at the wrong symbol and compresses worse than not running the
  model at all.
* **Prior.** PPMD's 256-way distribution is scattered into the 205 slots; the
  model's 205-way output is gathered back, floored at 10⁻⁶ and renormalized
  over the bytes the file uses, and handed to the mixer alongside PPMD's own
  distribution.
* **Pieces.** Contexts are reset every 131072 bytes; the byte at a boundary is
  coded from PPMD alone.
* **Training** (`TF_TRAIN=3`). The fp32 engine keeps every parameter as a
  plain float and has a backward pass through all of the above; coder0 takes
  one AdamW step per byte on the loss of the byte just coded. The recurrent
  state and the KV rings are treated as constants for that step (truncation
  at horizon 1), which keeps the update exact for everything position-local
  and makes it checkable against finite differences. This is what turns the
  model from an enwik9 specialist into something that adapts to the file at
  hand: frozen, it loses book1 to the LSTM by 10 KB; trained, it wins.
