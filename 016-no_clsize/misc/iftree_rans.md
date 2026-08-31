# The if-tree, tested with rANS

`dec_vectorize.md` §9 measured Shelwien's generated if-tree against the range
coder and found **1.63× at 16 lanes** -- 255 real branch sites, one per context
node, so the hardware predictor learns each node's own bias and `cty[c]` is a
compile-time constant address. It left one obstacle open: in this codebase byte
*m*'s bit *j* lives in lane `m*8+j`, so a byte's eight bits are eight different
coder lanes, where the bench had one lane hold a whole byte in registers.

`iftree_rans_gen.py` and `iftree_rans.cpp` are the rANS twin of that probe --
same coder as `rans.inc`, same lane assignment as `model1.inc`, the same
synthesised 256-state FSM counter `rans_shapes.cpp` uses. enwik8, 100 MB,
`-march=native`, min of 3:

| shape | enwik8 | 20 MB urandom |
|---|---|---|
| **A** `model1.inc`'s wavefront, indexed `cty[lctx]` | **57.95** | **47.39** |
| S the same walk, byte-sequential (the ILP control) | 56.28 | 47.43 |
| T 255-node if-tree, coder step inline at each node | 31.98 | 14.54 |
| U 255-node if-tree, bits only, updates after the byte | 35.01 | 18.09 |
| P2 if-tree for 2 levels, then the indexed walk | 47.73 | |
| P3 if-tree for 3 levels | 45.20 | |
| P4 if-tree for 4 levels | 40.06 | |
| SD S with the bit made to wait for the multiply | 50.18¹ | |
| TD T with the bit made to wait for the multiply | 29.34¹ | |

¹ measured in a later run whose A/S were 49.92/50.02; compare within a run.

**It loses, and not marginally.** Every variant is below the plain walk, and
the partial trees are monotone in depth: two levels already cost 15%, and each
further level costs more. There is no depth at which it pays.

## The lane arrangement is not the obstacle

That was the open question and the answer is no. **S** is the control: the same
indexed walk, but byte-sequential like the trees, so it gives up the
wavefront's `NB = RCNUM/8 = 2` independent byte chains exactly as a tree must.
It costs **3.3%**. The trees cost 40-45% against it, so what they lose is not
the interleaving.

That also weakens §9's other suggestion in passing -- that reassigning lanes so
one lane holds a whole byte would be worth having for the extra model chains.
Here two chains against one are worth 3.3%, which is not much to reorganise a
stream format for.

## What the controls rule out — including my first explanation

Four controls, all within this one probe so nothing but the named variable
moves:

| control | question | answer |
|---|---|---|
| **S** byte-sequential indexed walk | is it the lost wavefront ILP? | **no** — 3.3% |
| **SD/TD** the bit made to wait for the multiply | is it where the multiply sits? | **no** — see below |
| spill counts from the listing | is it register pressure? | **no** — U spills 1.4% |
| **P2/P3/P4** partial trees | is there a depth that pays? | **no** — monotone |

**The multiply.** My first reading of this result was that the range coder puts
a multiply between the model's `p` and the decoded bit -- `code >=
(range>>SCALElog)*p` -- so speculating past that branch buys five cycles,
while rANS's `(x & (M-1)) >= p` resolves the cycle `p` arrives and has nothing
to hide. The multiply is certainly *there* in rANS's step, and it is certainly
off the branch's path; the listing shows the compare using `p` and the masked
state while the `imull`'s result goes only to the update:

```
    movl  _ZL4cty_+4(%rip), %ecx     ; the counter, at a constant address
    movzwl %cx, %eax                 ; p
    andl  $32767, %edx               ; s = x & (M-1)
    shrl  $15, %r15d
    imull %eax, %r15d                ; a = p*(x>>15)   -- beside the branch
    cmpl  %eax, %edx                 ; s vs p          -- does not use %r15d
    jae   .LBB3_26
```

**But that is not why the tree loses, and the experiment says so.** SD and TD
are S and T with an empty `__asm__` that forces the branch to wait for the
multiply, exactly as the range coder's does -- one dependency added, nothing
else. If the multiply's position were the mechanism, putting it on the path
should cost the flat walk (which cannot speculate past the branch) more than
the tree (which can), and the ratio should move. It does not move:

| | walk | tree | tree/walk |
|---|---|---|---|
| as written | S 50.02 | T 30.13 | 0.602 |
| bit waits for the multiply | SD 50.18 | TD 29.34 | 0.585 |

The walk does not care at all, and the tree gets marginally worse. The
explanation was wrong.

**Nor is it code size**, which was the next guess. U's body is 7,675
instructions against the walk's 756 -- but the range coder's *winning* tree is
**12,516**, larger still. A big body is not what decides it.

## What is actually left, and what this probe cannot settle

What the controls leave is the plainest reading: the tree carries a large cost
-- 285 conditional branches in U, 1,022 in T, against 18 in the walk -- and it
can only win where the walk it replaces is more expensive than that cost. In
this probe the rANS walk is about 20 ns/byte and the trees land at 30-31.
`iftree_bench`'s range coder walk is about 52 ns/byte on the same box and its
tree lands at 38, so there the same trade clears.

**That is where the honest account stops.** The two benches do not differ only
in their coder: `iftree_bench`'s model is a shift-and-clamp counter with no
table where this probe uses the FSM lookup `counter.inc` actually has, and its
`decA` is a non-inlined call per byte per lane that reloads `code`, `range` and
the cursor from globals every time, where this probe keeps a block's state in
one function. Its baseline is inflated by its harness. So the cross-bench
comparison shows that the tree wins there and loses here, and does **not**
establish that the coder is the reason.

Settling that would take one more experiment: the same probe with the range
coder dropped in beside rANS, same model, same lane arrangement, same harness.
Until then the useful conclusion is the narrow one -- **for this decoder, as it
is built, the if-tree loses in every shape and at every depth tried, and the
three obvious explanations for why are all ruled out.**

## Reproducing

```
cd misc && python3 iftree_rans_gen.py
clang++ -O3 -march=native -std=c++17 -o ir iftree_rans.cpp && ./ir ../../enwik8 3
```

The generated `rans_tree_*.h` are build products and are not committed.
