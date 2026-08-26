# The class count, and the shape of the model

Two experiments: whether mrpc's class-count heuristic can be improved, and
whether the generalized Gaussian it codes with can usefully be given more
parameters.

**Results.** The class count: `-n` added, heuristic unchanged -- three
replacements were measured and every one was worse on average than MRP's own
rule, for a reason worth knowing. The model: one new parameter, a tail weight
that unties the tail from the peak, worth **-0.94% mean and -1.44% total**
over the corpus and **-3.67% on `t24.bmp`**, at no measurable cost in time.

## Before anything else: the search is chaotic

Nothing here can be measured on one image. Two demonstrations.

**A compiler inlining difference moves the output 0.12%.** The same source,
unchanged, compiled as one translation unit instead of two under
`-march=native`, codes `t24_gray.bmp` to 55,508 bytes instead of 55,576.
Floating-point contraction differs by an ULP somewhere in the fit, and the
optimiser lands somewhere else.

**Output as a function of class count is bimodal, not smooth.** On a 160x128
tile:

| classes | 12 | 14 | 16 | 18 | 20 | 22 | 24 | 26 | 28 | 30 |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| bytes | 22178 | 21943 | **16977** | **17358** | **16798** | **17417** | 22289 | 22095 | 21351 | **17721** |

Adjacent settings differ by 30%, and the two clusters are 5 kB apart. That is
not a cost-benefit curve with noise on it; it is two attractors.

So every number below is measured over a **24-image corpus** of tiles cut
from the three test images (eight 256x256 from the 4096x512 RGBA one, four
160x128 from `t24`, four 128x128 from `PIA13882`, and eight 128x96 8bpp grey
and paletted tiles), and scored as the **mean of per-image percentage
changes**, so an 8 kB tile counts as much as an 85 kB one.

## 1. The class count

### The option

```
  -n <n>    use <n> predictor classes (2..63) instead of the number
            the image size suggests
```

and `mrpc_opts.num_class` does the same through the C API. `0` keeps the
automatic choice. It skips the automatic machinery entirely, which is what
makes it useful for exactly the kind of sweep below.

### The old rule is wrong in both directions

MRP's rule is `10.4e-5 * pixels + 13.8`, linear in the pixel count with a
constant term that dominates anything small. Swept against the truth, on
crops of two images at matched sizes:

| image | pixels | old rule | best | best vs old rule |
| --- | --- | --- | --- | --- |
| `t24` 64x64 | 4,096 | 14 | 6 | **-11.47%** |
| `t24` 96x96 | 9,216 | 14 | 8 | **-11.21%** |
| `t24` 128x128 | 16,384 | 15 | 16 | +1.70% |
| `t24` 192x192 | 36,864 | 17 | 45 | **-27.56%** |
| `t24` 320x240 | 76,800 | 21 | 63 | **-13.94%** |
| `PIA` 64x64 | 4,096 | 14 | 4 | -4.51% |
| `PIA` 96x96 | 9,216 | 14 | 4 | -1.78% |
| `PIA` 128x128 | 16,384 | 15 | 4 | -1.17% |
| `PIA` 192x192 | 36,864 | 17 | 6 | -0.52% |
| `PIA` 256x256 | 65,536 | 20 | 16 | -0.21% |

It gives a 64x64 crop fourteen classes where six is better, and a 192x192
crop seventeen where forty-five is better.

### Size cannot predict it

Look at the two 192x192 rows. Same pixel count, same rule, and one wants
**45** classes while the other wants **6**. No function of `W*H` can serve
both. Whatever sets the right number, it is not the size.

### What does bound it

The fit. Each class gets its own linear predictor per component -- `nt`
coefficients estimated from the `W*H/n` pixels that land in that class -- so
choosing `n` is really choosing how many samples each coefficient gets.

Instrumenting the encoder to report how many classes survive shows what
happens when that number gets small. On the 160x128 tile, at `-n 24`:

```
[1: 0] ... = 27067 B  used 14/24
[1: 5] ... = 25048 B  used  3/24
[2: 0] ... = 23804 + 600 side B  used 2/24 leaves 2
```

Two of the twenty-four classes are ever used, and the quadtree collapses to
two leaves -- the whole image under one predictor. At `-n 20` the same tile
reaches `15753 + 1198 side` with fifty leaves. 24 classes x 34 taps over
20k pixels is 25 samples a coefficient; the per-class fits chase noise, the
class search then cannot tell the classes apart, and everything collapses.

So a rule built on samples-per-coefficient, `n = W*H / (SPC * nt)` with
`nt` the typical tap count, should beat one built on pixels alone. It does
not.

### None of the replacements is better

Over the corpus, against MRP's rule, mean of per-image percentages:

| | MRP's rule | SPC=37 | SPC=75 | SPC=120 | trial |
| --- | --- | --- | --- | --- | --- |
| mean | 0.00% | **+0.24%** | +0.43% | +1.82% | +0.38% |
| best case | — | -4.32% | -8.28% | -2.88% | -9.67% |
| worst case | — | +3.28% | +24.86% | +23.47% | +21.57% |

(`+` is larger, i.e. worse.) Every one of them is worse on average, and the
reason is in the "best case" and "worst case" columns rather than the mean:
they all beat MRP's rule on some images by a lot, and all lose on others by
more. The trial -- which actually *measures* rather than guessing -- has both
the best single result (-9.67%) and a +21.57% disaster.

That is the bimodality again. A rule that moves the class count is a rule
that sometimes moves it across the boundary between the two attractors, and
the loss when it does swamps the gain when it does not. **MRP's rule is not
good at predicting the optimum. It is good at staying out of trouble**, and on
this codec that is worth more.

So the default is unchanged, and `-n` is how you do better on an image you
care about -- where a sweep is cheap and you can see which mode you are in.
The real fix is not a better heuristic; it is whatever makes the class search
stop collapsing.

### Can it go past 63?

Yes, and it is not worth it.

Nothing in the codec needs the cap to be 63. Every array that depends on it
grows linearly and none of them is bit-packed; the only real ceiling was that
the class map was a `char*`, and on x86 `char` is signed, so a class index
above 127 read back negative. That is now a `byte*`, which moves the ceiling
to 255 and is byte-identical at the default cap. `-DMRP_MAXCLASS=255 -n 200`
round-trips.

What it buys, on the two images whose optimum was clipped at 63:

| classes | 48 | 63 | 80 | 100 | 127 | 160 | 200 | 255 |
| --- | --- | --- | --- | --- | --- | --- | --- | --- |
| `t24.bmp` | 86424 | 81591 | 88602 | 82157 | **80195** | 88943 | 90486 | 83360 |
| `t24_pal.bmp` | 42706 | **41170** | 44033 | 43481 | 42468 | 42579 | 44414 | 41467 |

`t24.bmp` does best at 127, 1.7% under its best-at-63 -- and the row swings
13% between adjacent settings, so that 1.7% is smaller than the noise it sits
in. `t24_pal.bmp` does best at 63 and every larger setting is worse. It is
the bimodality again, and at these counts the sampling is coarse enough that
picking 127 over 63 is picking a lottery ticket.

And the case the cap was most obviously binding on -- `20000171A.bmp`,
4096x512, 2.1M pixels, where MRP's rule extrapolates to **232 classes** and
gets 63 -- turns out not to care at all:

| classes | bytes | vs 63 | encode |
| --- | --- | --- | --- |
| 63 | 2,751,355 | — | 108 s |
| **96** | **2,749,300** | **-0.075%** | 128 s |
| 127 | 2,752,511 | +0.042% | 133 s |
| 160 | 2,750,949 | -0.015% | 178 s |
| 200 | 2,754,516 | +0.115% | 203 s |
| 232 | 2,756,781 | +0.197% | 177 s |

0.27% across a 3.7x range of class counts, and the best is 96 -- 0.07% under
63, for 19% more encode time. 232, which is precisely what the heuristic
asks for, is the worst row in the table.

Two things follow. The cap is not costing this image anything, so there is
nothing to collect by raising it. And MRP's rule is wrong at the top end as
well as the bottom -- it wants 232 where the truth is 63 to 96 -- it is
simply that the cap has been quietly saving it.

It is also the image where the class count matters *least* of everything
tested: 0.27% here against 30% swings on a 160x128 tile. At 2.1M pixels every
class has 33k pixels and 850 samples a coefficient even at 63, so the fits
are all well determined and the class map is doing real work instead of
running a lottery. The instability is a small-image problem.

The costs are real, though. Both searches are linear in the class count, so
255 classes is about four times the encode. And three buffers scale with the
cap rather than the count -- the group histogram, its prefix sums and the
threshold DP's -- so the *cap* alone costs memory whether or not the classes
get used:

| `MRP_MAXCLASS` | `cbuf` | `d_hist` | `d_cum` |
| --- | --- | --- | --- |
| 63 | 16.6 MB | 16.6 MB | 16.6 MB |
| 127 | 33.4 MB | 33.4 MB | 33.4 MB |
| 255 | 67.1 MB | 67.1 MB | 67.1 MB |

So the default stays at 63. It is also part of the format -- the class count
is coded over `MRP_MAXCLASS+1` symbols -- so a stream written by a build with
a different cap will not decode.

### And the trial, which is off by default

The alternative to predicting the count is measuring it: run one pass of each
optimisation loop at each candidate count, and take the one whose cost --
side information included, which is what pays for a class -- comes out lowest.
`-DCLASS_TRIAL=1` builds that in.

It has to include a pass of the *second* loop. The first loop segments into
flat 8x8 blocks with no quadtree, so it cannot see what a large class count
buys, and a trial that stops after it ranks every candidate within a percent
of every other and then picks the smallest -- measured, on `t24_pal.bmp` a
loop-1-only trial chose 10 classes where the sweep wanted 63.

With both loops it agrees with a full sweep on four of five images tested and
loses 0.39% on the fifth. It costs one pass of each loop per candidate, which
roughly doubles a small encode.

## 2. The shape of the model

### sigma is not a free parameter

The scale the coder uses comes from one place: `SIGMA[16]`, a hardcoded
geometric ladder from 0.15 to 30.89. The threshold DP chooses which
activities land in which group; it does not choose what sigma that group has.
The shape is chosen per (component, group) and transmitted; the scale never
is.

Fitting a free generalized Gaussian to the actual residuals of each
(component, group) on `t24.bmp` shows what that costs:

```
comp 0 group 15   10042 sym   sigma  89.1   shape 0.43
comp 1 group 15    8856 sym   sigma 203.7   shape 0.34
comp 2 group 15    7502 sym   sigma 377.9   shape 0.30
comp 2 group  0   58247 sym   sigma   0.21  shape 1.19
```

The top group wants a scale of 90 to 378 where the ladder stops at 30.89, and
the bottom group -- a quarter of the image -- wants 0.21, between the rungs at
0.15 and 0.26. `SIGMA_LO`/`SIGMA_HI` now generate the ladder so both ends can
be moved; the defaults reproduce MRP's own numbers to the digit.

### The tail is tied to the peak

A generalized Gaussian has one shape parameter, and it sets the peak and the
tail together: heavy tails only come with a sharp peak. Real residuals do not
oblige. Fitting `(1-w)*GG + w*uniform` per (component, group), against what
mrpc actually charged for the same symbols:

| image | free sigma+shape | + a shift | + asymmetric scale | **+ a uniform floor** | + a second scale |
| --- | --- | --- | --- | --- | --- |
| `t24.bmp` | -0.85% | -0.45% | -0.72% | **+3.95%** | +3.00% |
| `t24_gray.bmp` | -0.83% | -0.80% | -0.84% | **+1.40%** | +0.99% |

(net of the side information, generously costed at two bytes a parameter;
`+` means smaller than mrpc as it is.) Freeing the scale and shape on their
own buys nothing -- the two-pass search has already found the best member of
the family. A shift buys nothing: the residual is conditionally unbiased. An
asymmetric scale buys nothing: it is symmetric. **Untying the tail from the
peak is the one degree of freedom the family is missing.**

### What that looks like in the codec

mrpc already transmits an index per (component, group) selecting one of
`NUM_PMODEL` models, and `pmodel_cost` already searches all of them. So the
extension is to make the family two-dimensional and let the existing search
pick both coordinates:

```
NUM_PMODEL = NUM_SHAPE * NUM_TAIL
idx  ->  shape = 3.2*(idx%NUM_SHAPE + 1)/NUM_SHAPE
         tail  = TAILW[idx/NUM_SHAPE]
freq[i] = (1-tail)*norm*pdf[i] + tail*budget/511 + MIN_FREQ
```

Nothing structural changes: the index is coded with the same model over a
larger alphabet (two more bits per group, 16 bytes an image), the search loops
are already bounded by `NUM_PMODEL`, and the OpenCL cost table just gets
taller. `NUM_TAIL=1` is byte-identical to the codec without it.

### What it is worth

Over the sixteen small tiles of the corpus, against `NUM_TAIL=1`:

| | 1 | 3 | **5** | 8 |
| --- | --- | --- | --- | --- |
| total | 0.00% | -0.64% | **-1.44%** | -1.50% |
| mean | 0.00% | -0.50% | **-0.94%** | -0.61% |
| best case | — | -3.93% | -5.15% | -6.93% |
| worst case | — | +0.58% | +2.31% | +5.77% |
| median | — | +0.03% | +0.05% | +0.06% |

The median is zero, which says what the gain is made of: most images do not
care, and a few care a lot. Per tile, `NUM_TAIL=5`:

| | RGB | | | | quantised | grey | palette |
| --- | --- | --- | --- | --- | --- | --- | --- |
| | `t24_0` | `t24_1` | `t24_2` | `t24_3` | `pia_*` | `t24g_*` | `t24p_*` |
| | -1.72% | -5.15% | -4.57% | -4.30% | +0.05% | +0.06% | +1.76% / -2.80% |

Photographic RGB gains 2-5% consistently; the quantised space imagery and
grey are untouched, which is exactly what the entropy study said (their
residual really is generalized-Gaussian, so there is nothing for a free tail
to fix); the palette tiles are noise in both directions, as they are for
every change.

`NUM_TAIL=5` is the default. It **costs nothing measurable in time** -- 6.2 s
against 6.3 s on `t24.bmp` -- because `pmodel_cost` is under a percent of the
encode even multiplied by five. It costs about 42 MB of model tables and
31 MB of cost tables.

On the full images rather than the tiles: `t24.bmp` 99,245 -> **95,602 bytes
(-3.67%)**, `t24_gray.bmp` +0.36%, `t24_pal.bmp` +1.10%, `PIA13882_crop256`
+0.005%.

### What did not work

**Wider or finer sigma ladders.** `SIGMA_HI` raised to 60 or 120, and
`MRP_GROUP` raised to 24 or 32 with the ladder stretched to match, measured
on four images: between -0.2% and +15%, in both directions, with no pattern.
The fitted scales say the range is wrong, but simply moving the rungs does not
collect the difference -- the threshold DP compensates by reassigning
activities, and what is left is the search landing somewhere else. Worth
revisiting only with the scale made a *transmitted* per-group parameter, which
is the same trick the tail axis uses and a much larger family.

**Regularising the least-squares fit.** A ridge term on the normal equations,
swept over five orders of magnitude, is worse everywhere: it shrinks the sum
of the coefficients, which darkens the class -- the effect `FitPredictor`'s
own rounding compensation exists to prevent.

**Capping the fit weighting.** MRP weights each sample by `1/sigma^2`, which
across the ladder is a ratio of 42,400 to 1 between the quietest group and the
noisiest, so a handful of flat pixels can outvote a class -- a plausible cause
of the collapse above. Capping that ratio measured as noise over the corpus
and is not in the tree. It is still the right place to look if the collapse is
ever chased further: on the 160x128 tile at 24 classes an *unweighted* fit
escapes the bad attractor (18,019 bytes instead of 22,289), while a merely
capped one does not, so whatever is going on is not a smooth function of the
weighting.

## Knobs

| | default | what it does |
| --- | --- | --- |
| `-n <n>`, `mrpc_opts.num_class` | 0 | class count; 0 = automatic |
| `CLASS_TRIAL` | 0 | 1 = measure the class count instead of predicting it |
| `MRP_CLASS` | 0 | pin the class count at build time |
| `NUM_SHAPE` | 16 | generalized-Gaussian shapes in the family |
| `NUM_TAIL` | 5 | tail weights the family is multiplied by; 1 = the codec as it was |
| `SIGMA_LO`, `SIGMA_HI` | 0.15, 30.89 | the ends of the scale ladder |
| `MRPC_MINSAMP`, `MIN_SAMP` | 0 | least-squares samples a coefficient a class must hold to survive the search; 0 = off |
| `MRPC_CRANGE` | — | pin the transmitted coefficient clamp instead of choosing it |
| `MRPC_SSE` | — | pin the residual correction instead of trialling it |
| `MRPC_PROG` | — | force the progress trace on, whatever the caller asked for |

## What the coefficient measurements turned into

§6 of `MODEL-IMPROVEMENTS.md` read the tap experiments here as saying the
coefficient quantiser was the binding constraint, and proposed retuning the
coder for a wider range as an *enabler*. It is a gain in its own right, and
only on single-component images: `+-4` measures -4.2 % on `piap_0` and -3.0 %
on `t24p_0` while costing +4.4 % on one 24-bit tile. The clamp is now chosen
per image and transmitted rather than compiled in. What it does *not* do is
make more taps pay -- that part of §6 was right about the diagnosis and wrong
about the remedy, and `MODEL-IMPROVEMENTS.md` §14 has the rest.
