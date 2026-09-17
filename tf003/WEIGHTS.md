# Optimizing the transformer's weights for compression

The question: the weights in `6m-q4-fp32*.tfwc2` are static int4 values, and
every one of them could be nudged by ±1 and the file recompressed. Some nudges
would surely make the compressed output smaller. Is it feasible to find them by
searching - an OpenCL kernel computing the code length under thousands of weight
variants at once, sharing the inputs (the prior, the bytes) that do not depend
on the weights? Or should it be gradients, given that the weights are discrete?
And what is actually the best route to a smaller file?

Short version. **The search is feasible as engineering and hopeless as a method
for the body of the model, by a factor of about two million against a
gradient.** A gradient pass *is* the evaluation of every single-weight
perturbation at once, to first order, and one already exists in this tree in
fp32 (`TF_TRAIN=3`), measured at 5 % on the same objective. Discreteness is a
real problem but a solved one: it is the whole subject of post-training
quantization, and the standard answers (straight-through estimation, and
Hessian-based rounding - OBQ/GPTQ) need gradients or activations, not searches.
The one place a code-length search is the right tool is the last layer, where
a cached forward pass makes every candidate cost O(tokens) - and there it is
excellent, and cheap enough not to need a GPU at all. Finally, at anything
below a few MB of input the weights are the file, not the data: 2.8 MB of
container against 25 KB of `book1wrt[:64K]`, so "best compression" is a
question about the container first and the cross-entropy second.

Everything below is measured on this tree's model and hardware unless it says
otherwise.

Contents

1. [The numbers](#1-the-numbers)
2. [Searching over weights: what the OpenCL plan really costs](#2-searching-over-weights-what-the-opencl-plan-really-costs)
3. [Gradients, and why discreteness does not rule them out](#3-gradients-and-why-discreteness-does-not-rule-them-out)
4. [What "best compression" means when the weights ship too](#4-what-best-compression-means-when-the-weights-ship-too)
5. [What to actually do, in order](#5-what-to-actually-do-in-order)
6. [Where an OpenCL kernel earns its place](#6-where-an-opencl-kernel-earns-its-place)

---

## 1. The numbers

**The model** (`tf/PORTING.md`, `cpp_infer/SPEC.md`): 205 tokens, residual
width 192, 12 layers - 9 Kimi-style linear-attention (KDA) layers with a
recurrent state, and 3 sliding-window attention layers over a window of 1024
positions, 3 heads of 64. Weights are int4 with a bfloat16 scale per output row;
activations are int8.

| | count | in the container |
| --- | ---: | --- |
| int4 weights | 5 868 864, in 111 tensors | 3.76 bits each (adaptive, per class): 2.75 MB |
| bf16 row scales | 26 083 | ~40 KB |
| raw f32 (biases, conv taps, gates, decay rates) | 28 281 | ~50 KB as bf16 |
| `rope.sin`/`rope.cos` | 2 × 4 194 304 | not stored, recomputed |
| **`6m-q4-fp32-t1lambda1.tfwc3`** | | **2 815 630 B** |

The int4 symbol distribution, from the shipped blob, which sets the container
cost of every candidate move:

| symbol | 0 | ±1 | ±2 | ±3 | ±4 | ±5 | ±6 | ±7 |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| probability | .116 | .101 | .093 | .075 | .059 | .044 | .018 | .051 |
| cost, bits | 3.11 | 3.30 | 3.43 | 3.72 | 4.09 | 4.49 | 5.78 | 4.30 |

Nearly flat. The quantizer sets each row's scale to max|w|/7, so every row's
largest weight lands on ±7, which is why ±7 is *cheaper* than ±6. Order-0
entropy is 3.756 bits/weight; zmix's per-class adaptive coder gets 3.78, and
its λ\* training - a penalty on exactly this entropy - moved it from 3.786 to
3.756, worth 22 KB.

**Compute per token**, forward: about 2 FLOP per weight for the matmuls
(11.7 M), plus the window attention (~2.4 M) and the KDA recurrence (~0.4 M):
**~15 MFLOP/token**. A backward pass is about twice that again. Over a 64 K
evaluation window: **1 TFLOP forward, 3 TFLOP forward+backward.**

**Measured here** (4-core Xeon, AVX2 build):

| | per token | 64 KB |
| --- | ---: | ---: |
| int4 engine, forward (`coder0`'s transformer share) | 0.25 ms | 16 s |
| fp32 forward + backward + AdamW, `TF_TRAIN=3`, batch 1 | ~9 ms | 36 s per 4 KB |
| … batch 16 | ~4.5 ms | 18 s per 4 KB |

**What a gradient already buys**, from the `TF_TRAIN=3` calibration in the
header of `transformer.inc` (online fine-tuning of the pretrained model,
lr 3e-6, transformer + PPMD pipeline at the time):

| | frozen | trained online |
| --- | ---: | ---: |
| `book1000` | 1 735 | 1 671 (−3.7 %) |
| `book1[:16384]` | 6 002 | 5 700 (−5.0 %) |
| `book1[:65536]` | 21 086 | 20 038 (−5.0 %) |

So the premise is right: for a given file the shipped weights are about 5 % off
the best this architecture can do on it. The question is only how to find that
5 %.

**What retraining bought zmix**, through this tree's coder0: `t1lambda1` vs the
gen-7 blob, 25 396 → 25 333 on `book1wrt[:64K]` (−0.25 %); their own figure is
−10 318 B at their 20 MB tier.

**Per-variant state**, which is what "how much memory per weight variant"
comes to. The weights are shared, but from the changed layer onward nothing
else is: every variant needs its own residual stream, its own KDA states and
its own KV rings, and those are what the recurrence carries forward.

| | per variant |
| --- | ---: |
| KDA state, 9 layers × 3 heads × 64 × 64 fp32 | 442 KB |
| attention KV rings, 3 layers × 1024 × 2 × 192 fp32 | 4.7 MB |
| conv histories, norms, logits | small |
| **total** | **~5 MB** (half in fp16) |

## 2. Searching over weights: what the OpenCL plan really costs

Take the plan at its strongest: a batched forward pass, thousands of variants
in flight, and the layers *before* the changed weight computed once and shared
(their residual streams cached: 12 × 192 floats × 64 K tokens = 600 MB, fine).

**Sharing buys a factor of two, not more.** A weight in layer L changes that
layer's output at every position, and from there the residual stream, every
later layer, and - through the KDA state and the KV rings - every later
position. Nothing after L is shareable. Averaged over the layers, a variant
recomputes half the network: ~0.5 TFLOP per variant on a 64 K window.

**One sweep is 6 EFLOP.** 5.87 M weights, two directions each: 11.7 M variants
× 0.5 TFLOP. A GPU quoted at 20 TFLOPS fp32 would need 3.4 days at 100 %
utilization, and the model is a recurrence - each variant is a sequential
64 K-step scan of small ops, so utilization comes only from breadth, i.e. from
thousands of variants resident at 5 MB each. A realistic 5 TFLOPS makes it two
weeks. And a sweep is one coordinate-descent pass; the fp32 gradient result
above took thousands of steps.

**Cheaper proxies do not rescue it.** A shorter window (4 K tokens) is 16×
cheaper and 16× more prone to fitting the window: a ±1 int4 step is about a
seventh of the row's maximum, a coarse move, and choosing 5.87 M of them by
their effect on 4 K tokens is memorizing those tokens. The evaluation has to be
on data at the scale of what will be compressed.

**Against the gradient.** One forward+backward pass over the same window is
3 TFLOP - well under a second on that GPU, under a minute on this CPU - and
returns ∂L/∂w for all 5.87 M weights at once. That is the first-order value of
every one of the 11.7 M variants, from one pass: **the gradient is the batched
perturbation experiment, done analytically.** The ratio is 6 EFLOP to 3 TFLOP,
two million to one, and the gradient's answer is the one the search would have
converged to anyway, to first order.

The engineering, for the record, is real but bounded: the kernels are an int4×
int8 (or fp32) GEMV, rms-norm, a causal conv with SiLU, the KDA rank-1 state
update with decay, windowed attention with RoPE, a logit softcap and a softmax.
All of it ports. It is the method that does not.

## 3. Gradients, and why discreteness does not rule them out

The objection is fair: the weights are 15 integer levels, a gradient is a
direction in a continuous space, and a step of "one level" is coarse. Four
things answer it, and together they are the whole discipline of quantized
training and post-training quantization.

**3.1 The straight-through estimator (QAT).** Keep a latent fp32 copy of every
weight. The forward pass uses `round(w/s)·s`; the backward pass pretends
rounding is the identity and applies the gradient to the latent copy. The
latent weights drift continuously and the quantized ones snap when a rounding
boundary is crossed. This is what makes the −5 % of `TF_TRAIN=3` *shippable*:
as it stands that run trains fp32 weights that are then requantized by
`quantize_row_int4` - which keeps a row's old scale when it still covers the
row, so an update that does not cross a level boundary simply vanishes on save.
Train the quantized forward instead of the fp32 one and the model absorbs the
update on its own grid. It is a small change to `fp32_model.inc`: fake-quantize
the weights on the way into each matmul, and nothing on the way back.

**3.2 Gradient-guided moves.** Even without latent weights, ∂L/∂w says what a
±1 move is worth: ΔL ≈ g·s·δ. One backward pass ranks all 11.7 M candidate
moves; a search then only has to *verify* the top few thousand, in batches -
which is section 6. The first-order estimate is poor for a step this coarse,
so it is a filter, not an answer; but it turns a 6 EFLOP search into a few
TFLOP of confirmations.

**3.3 Hessian-based rounding: OBQ / GPTQ.** This is the direct answer to "the
weights are discrete." For one linear layer with inputs X (the calibration
activations, N × d_in), quantizing the row w to ŵ costs ‖(w − ŵ)X‖², a quadratic
with Hessian H = XXᵀ - **192 × 192 or 768 × 768 per layer, tiny**. OBQ rounds
one weight at a time and moves the *unrounded* weights of the row to cancel the
error, by the closed form

    δw = −(w_q − round(w_q)) / [H⁻¹]_qq · [H⁻¹]_{:,q}

GPTQ is the same in a column order that lets it run over all rows at once. It
is the standard for int4 language models and it is what the C++ in this tree
can already feed: `fp32_model` dequantizes the weights, the forward pass yields
X for every layer, the quantizer in `weights_write.inc` writes the result. **It
needs no backward pass at all** - one forward pass over a calibration window,
a small matrix inverse per tensor, and it makes the rounding decisions with
second-order information that the ±1 search would have to discover by trial.

**3.4 Learned rounding (AdaRound).** Between the two: each weight gets a
continuous parameter deciding "round up or down", trained by gradient with a
regularizer that pushes it to 0 or 1, then snapped. It optimizes the layer's
reconstruction like GPTQ but through the gradient like QAT.

The first-order caveat is real and is exactly why 3.3 exists: a ±1 step at
scale max|w|/7 is not small, and the gradient alone will propose moves that a
Hessian would veto. What none of that argues for is a search in place of
either.

## 4. What "best compression" means when the weights ship too

The objective is not the cross-entropy. It is

    L = bits(data | weights) + bits(weights)

because the decoder needs the weights and the benchmark counts them (zmix's
Form 1 counts the container *twice*, once in the compressor and once in the
archive). Which term matters depends entirely on how much data there is:

| input | data, compressed | container | ratio |
| --- | ---: | ---: | ---: |
| `book1000` (4 KB) | 1.8 KB | 2.8 MB | 1 : 1 550 |
| `book1wrt[:64K]` | 25 KB | 2.8 MB | 1 : 110 |
| `book1` (768 KB) | ~180 KB | 2.8 MB | 1 : 15 |
| enwik9-class (1 GB) | ~110 MB | 2.8 MB | 40 : 1 |

Three consequences.

**Below a few MB of input, the container is the whole problem.** No weight
adjustment that keeps 5.87 M weights at 3.76 bits each is worth anything on
`book1`: a 5 % gain on the data is 9 KB against a 2.8 MB fixed cost. The levers
that matter there are the ones that shrink the container - fewer weights, or
sparse ones (`qmat_sparse.inc` exists for a reason: a tensor that is mostly
zero codes at a fraction of 3.76 bits), or a smaller model - or no shipped
weights at all: `TF_LOAD_WEIGHTS=0 TF_TRAIN=3` trains from a seed at decode
time, and costs time instead of bytes.

**The container term is exactly computable per move, and it is small.** Moving
a weight one level toward zero saves 0.1–0.4 bits (table in section 1); moving
±7 to ±6 *costs* 1.5. Summed over every weight that could plausibly move, the
entropy lever is tens of KB - which is what zmix's λ\* got, 22 KB - not the
hundreds a pruning pass would get. A search *can* evaluate this term exactly and
cheaply, and it should, as the second half of the objective; it just does not
change the conclusion about the first half.

**Online adaptation is free in bytes.** The −5 % in section 1 was not a better
frozen model; it was the model training on the file as it coded it, encoder and
decoder alike, which adds nothing to the archive. Its cost is the fp32
backward pass at decode time: 36 s per 4 KB here, so 2 hours for `book1` -
which is precisely what a GPU is for, and a far better use of one than a
weight search.

**And the transformer is not the coder.** In coder0 its output is one input to
a per-node logistic mixer alongside the Tangelo prior, and the file's length is
the *mixed* code length. Training the transformer against its own
cross-entropy optimizes the wrong thing by a little; the mixer is
differentiable (`newton.inc` already computes ∂p/∂w for its own update), so
∂(mixed bits)/∂(transformer probs) is a few lines, and the existing fp32
backward would then train the model against what is actually being measured.
Retraining the model with the Tangelo prior in place of PPMD's - the mismatch
noted in `README.md` - is the same change from the other side.

## 5. What to actually do, in order

Ranked by expected gain per unit of work, with what each one needs.

1. **Measure the ceiling: `TF_TRAIN=3` through the current pipeline.** It runs
   today, on this tree, with the Tangelo prior. If the −5 % holds, that is the
   number every static-weight method is chasing, and it says how much of it is
   worth a static container at all. Cost: a build flag and two hours.

2. **Make the fp32 result shippable: QAT.** Fake-quantize the weights in
   `fp32_model`'s forward pass, so the −5 % lands on the int4 grid instead of
   evaporating at `SaveWeights`. Then the trained model is a 2.8 MB blob that
   coder0 loads as it is. Cost: a day in `fp32_model.inc`; evaluation is the
   same run as (1).

3. **GPTQ the requantization.** Whatever produces fp32 weights - (2), or a
   PyTorch retrain with the upstream training code as zmix did - round them
   with the layer Hessians rather than to nearest. One calibration forward
   pass, a 192² inverse per tensor, `weights_write.inc` to write it. Compare
   against round-to-nearest with coder0; this is where the discreteness is
   actually handled.

4. **Train against the mixed code length, with the Tangelo prior.** The
   gradient at the transformer's output from `newton.inc`'s math, fed into the
   existing backward pass. This aligns the model with the deployed objective
   and with the prior it is now given. Larger change, larger expected gain on
   the mixed result - the −0.25 % that survived the mixer in `README.md` is
   the symptom it treats.

5. **Polish the head by search - section 6.** After any of the above, a
   coordinate-descent pass over the unembedding and its scales against the
   exact objective, container term included. Minutes on a CPU.

6. **For small inputs, attack the container instead.** Magnitude pruning with
   the sparse kernels, then (3) to re-round what is left; or accept online
   training with no shipped weights. The container term, not the data term,
   decides these.

What is deliberately not on the list: a GPU search over the body's weights.
Everything it could find, (1)–(3) find first and cheaper.

## 6. Where an OpenCL kernel earns its place

Two jobs, and the first needs no GPU.

**The last layer, exactly.** With one forward pass cached - the final-norm
vector h_t (192 floats) and the logits for every token - a change δ to
unembedding weight w[o][j] moves logit o at every position by δ·s_o·h_t[j] and
nothing else. The new code length is a sum over tokens of one changed logit
through the softcap and one corrected log-sum-exp: **O(tokens) per candidate,
with no recurrence in the way.** The head is 39 360 int4 weights and 205
scales; both directions of all of them on a 64 K window is 5 × 10⁹ simple
operations - seconds on this CPU, negligible on a GPU - and the container term
(section 1's table) drops in for free. The same holds, at one more matmul, for
the final norm's gain. This is the one part of the model where "try every ±1
and keep the winners" is exact, cheap and correct, and it is worth doing after
any retraining, because the head is what the rest of the model's error lands
on.

**Batched verification of gradient-proposed moves.** A backward pass ranks the
11.7 M candidate moves (section 3.2); the top k, applied together, are one
variant; a forward pass over the window says whether they helped, and bisection
over the set attributes the ones that did not. That is a forward kernel with
thousands of *sequences* in flight rather than thousands of *weights*, which is
the batching a GPU is good at, and it is a few TFLOP per round rather than six
EFLOP. It is the OpenCL kernel of the original idea, put to the one use where
it multiplies a gradient instead of replacing one.

For scale: this session's machine has no GPU. The head polish and the
`TF_TRAIN=3` measurement both run without one; QAT and GPTQ are a day each in
the C++; the mixed-objective training is the larger piece and the one with the
most headroom.
