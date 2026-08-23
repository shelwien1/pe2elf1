# IDX/

`IDX-FORMAT.md` is the declaration format. `IDX-NOTES.md` is what it means for
this codec. This is what the files here are.

## The framework

| file | role |
|---|---|
| `idx2inc.pl` | the generator: a `.idx` plus an optional `.inc` template becomes `MOD/*_h.inc` and `MOD/*_p.inc` |
| `import.pl` | folds `opt.pl`'s results back into the `.idx` sources by name |
| `opt.pl` | the optimizer: flips pattern bits in the tuning binary and keeps what shrinks the corpus |
| `sweep.py` | the clamp test — every parameter to both extremes, looking for a crash or a lost round trip |
| `roundtrip.sh` | proves `opt.pl` can reach every declared parameter and `import.pl` puts each back where it came from |
| `f2idx.py` | writes a float32 as the exact dyadic integer a `.idx` carries, plus the exponent its consumer applies |
| `annotate.py` | refreshes the readable comment above each mapping — run it after an `import.pl`, which moves the edges and leaves the comment stale |

`../mk.sh` drives the first of these; `../check.sh` runs the two-build
contract.

## The sources

`bmf-*.idx` are the declarations, and `bmf-*.inc` are the templates for the
modules that declare an `Index`. Both are hand-edited from here on. `MOD/` is
generated from them and is checked in, so the tree builds without perl.

## The one-time generators

`gen_*.py` and `idxgen.py` are how the port was done: each reads the constants
out of a source file, writes the `.idx` declarations, and rewrites the use
sites to name them. They are kept because they record what moved where and how
each value was derived — `gen_p2.py` still holds the float table the mantissas
came from, which is the only place the original literals survive.

They are **not** part of the build and should not be re-run. The `.idx` files
are the source of truth now, and several of them have been edited by hand
since; a generator would overwrite that with the state of the world when the
port reached it.
