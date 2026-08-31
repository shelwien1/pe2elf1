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

## The mechanism works. It just cannot pay for itself

The urandom column is the same control §9 used, and it says the same thing.
With branches at 50/50 the predictor is useless: T is **3.26×** slower than A
there, against **1.80×** on enwik8. So prediction is buying the tree a lot --
it just starts from far enough back that a lot is not enough.

The reason it starts so far back is a structural difference between the two
coders, and it is the whole result:

- The range coder's bit is `code >= (range>>SCALElog)*cty[ctx]`. A **multiply**
  sits between the model's `p` and the decoded bit, so `p` arrives, then five
  more cycles pass before the branch can resolve. Speculating past that is
  worth a great deal, and that is what §9 measured.
- rANS's bit is `(x & (M-1)) >= p`. The left side does not mention `p` and is
  ready before the counter load returns, so the bit resolves the cycle `p`
  arrives. **There is nothing between the model and the branch for speculation
  to hide.**

So the if-tree pays a mispredict to buy latency that rANS has already removed
by construction -- the same property that makes `RC_DEC_SPLIT` pointless here
(`rans_decode.md` §3) and makes the default decode-step form beat the textbook
one (§4). rANS's decode step is short on the model side, and every shape that
spends something to shorten it further has now lost.

It is also worth noting what the tree costs in the other direction: T inlines
the refill at all 255 nodes and comes to 2039 lines of generated body, against
U's 1019 with the coder step lifted out. U is faster than T by 9%, which is the
front-end paying for that footprint -- and both are far below a walk that fits
in a loop.

## Reproducing

```
cd misc && python3 iftree_rans_gen.py
clang++ -O3 -march=native -std=c++17 -o ir iftree_rans.cpp && ./ir ../../enwik8 3
```

The generated `rans_tree_*.h` are build products and are not committed.
