# ROSE-based refactoring tools for LLM agents

An LLM that refactors C or C++ spends much of its effort on facts that a compiler front end already
has: which declaration a name refers to, the type of an expression, where an expression starts and
ends, what an implicit conversion does, where a member sits in a struct.  This document proposes
tools built on this repository's ROSE and EDG (librose, and the `RoseRefactor` API that `rose-ren`,
`rose-using` and `rose-m2g` are built on) that give those facts to an LLM agent, together with the
rewrites that depend on them.

The examples come from `tools.zip`: the toolkit an agent wrote over a dozen rounds while turning
the decompilation of an image compressor, BMF.exe, into readable and portable C++.  The
decompilation is `subs1.hpp`, about 37,000 lines of Hex-Rays output, and every round had to keep
the program's output byte for byte.  The scripts' docstrings record each rule, what it declines,
and what it got wrong first, which makes them a good record of what such work needs, and of where
tools that read code as text break.

* [1. What the agent built](#1-what-the-agent-built)
* [2. What ROSE provides](#2-what-rose-provides)
* [3. Conventions for tools that an LLM calls](#3-conventions-for-tools-that-an-llm-calls)
* [4. Query tools](#4-query-tools)
* [5. Rewriting tools](#5-rewriting-tools)
* [6. Checking tools](#6-checking-tools)
* [7. What stays with the tests](#7-what-stays-with-the-tests)
* [8. Where to start](#8-where-to-start)
* [Appendix: the archive's scripts and the proposed tools](#appendix-the-archives-scripts-and-the-proposed-tools)

## 1. What the agent built

`tools/` has 85 Python scripts (15,600 lines) and 13 shell scripts.  Most scripts are one rule
each: rename identifiers, unify type names, delete useless casts, turn pointer arithmetic into
struct members, fold temporaries, remove `goto`s, turn stack-frame buffers into locals, decode data
tables.  The rest measure (`shape.py`), test (`asan.sh`, `fuzz.sh`, `x64diff.sh`), drive the rules
(`struct-sweep.sh`, `frame-sweep.sh`, `resign-drive.sh`), and check the checks (`proven.sh`,
`reads.py`, `readme.py`).

The method is sound, and the proposals below keep it:

* **One rule per tool, with a report mode and an apply mode** (`--list`, `--apply 3`, `--all`,
  `--only`), each tool idempotent.
* **Rules decline rather than guess, and say why.**  `degoto.py --why` sorts the `goto`s it
  declines into shared tails, jumps into or out of blocks, backward jumps and regions that other
  code enters; `unloword.py --declined` lists what stopped each candidate.
* **The tests decide.**  A change is kept only if the build passes and fifteen reference streams
  come out byte-identical (`test.sh`); AddressSanitizer, fuzzing and a 64-bit build check what the
  streams cannot see (`asan.sh`, `fuzz.sh`, `x64diff.sh`).  The drivers apply one candidate, run
  the tests, and keep the change or put it back, with a skip list so that a run can resume: "the
  rules propose and the gate decides".
* **Counts come from tools, not from documents**: "a number in a docstring is a measurement that
  stops being re-taken".  `sweep.sh` runs every tool on a copy of the file and requires each
  count to be zero; `proven.sh` replays each tool on old revisions to show that its zero can move.

What the agent had to build without a front end is the subject of the rest of this section.

### 1.1 A C++ parser, written as regular expressions

The scripts call Python's `re` 612 times.  43 of them import `structs.py`, whose helpers split the
file into function bodies, read the declared types of locals, split sums into terms and decide
whether a `*` is a dereference; the function-body splitter `bodies()` exists in four copies
(`structs.py`, `objects.py`, `retype.py`, `unframe.py`).  Their comments record what that parser
got wrong:

| What went wrong | Script | What a front end has instead |
|---|---|---|
| "`#pragma pack(push, 1)` above a struct has the parenthesis this is looking for -- which made deadcheck.py report a function called `pack` that nothing calls" | `structs.py` | Function definitions; directives are not code. |
| "`alignas(16) static uint8_t ctx_group_flags[32] = {` ... read twelve of the file's global tables as functions called `alignas` ... with `104 bodies` in section 1 where there are 92" | `structs.py`, `prune_unreachable.py` | A variable with an initializer is not a function. |
| "`__attribute__((noreturn)) void __exit_402E40(...)` gave `attribute__`"; `deadcheck.py` still keeps `attribute__` and `alignas` in a list of names to ignore | `structs.py`, `deadcheck.py` | Attributes are not names. |
| "a dereference split over three lines is one dereference"; "stopping at the first `]` finds only the inner one"; "`*v5` is a dereference and `16 * v5` is not" | `structs.py` (README) | Expression trees. |
| "A comment describing deleted code quotes it, so the first run found the very thing it was written to find, in the note saying that thing was gone" | `deadcheck.py` | Comments are not code. |
| "one of its members has a trailing comment, which a pattern anchored at the end of the line cannot see past" | `reframe.py` (README) | Declarations. |
| "`uint8_t  *` and `uint8_t *` are one type" (eight compile errors) | README | Types compared as types. |
| "Comparisons whose operand does not fit on its own line are left alone ... the first version put a closing parenthesis inside an argument list" | `explicitcmp.py` | The source range of every expression. |
| "its throwaway classifier took `\*+` ... turning six pointer stores into 'narrow' casts" | `uncastwidth.py` | Pointer types. |
| "`p + 3` steps three bytes when `p` is `char *` and twelve when it is `void **`" | `structs.py` (README) | The type of every expression. |

### 1.2 The compiler as an oracle, joined by line number

For questions about types, the scripts read GCC's warnings: `decast.py` and `uncast.py` act on
`-Wuseless-cast`, `explicitcmp.py` on `-Wsign-compare`, `resign.py` and `resign_group.py` on
`-Wsign-conversion` and `-Wconversion`, `retype_locals.py` on the conversions that need
`-fpermissive` (`strict.log`), `ptrwidth.py` on the warnings of a `-m64` build (`strict64.log`),
`unused.py` on `-Wunused-variable`.  That gave answers no pattern could, at three costs:

* **The join is by line and column**, so a log about another version of the file applies its
  answers to whatever is on those lines now.  `resign.py` "applied 17 changes to the wrong locals
  and put the warning count *up* by 43"; `buildlog.py` now stamps each log with a checksum of the
  source it was built from, and `reads.py` exists to find tools that read something other than the
  file they are given.
* **The compiler answers only what it warns about.**  "`(uint16_t *)` applied to a `uint8_t *` is
  not identical to anything, so the compiler is silent" (`unlayer.py`); "`(uintptr_t)p` is exact
  on every target ... the hop launders the pointer" (`ptrwidth.py`); `&p[-u]` with `u` unsigned
  "is well-formed and means two different things" on i386 and x86-64 (`negindex.py`).
* **It answers for one target**: "`-Wuseless-cast` answers for the target it was run against, and
  this file is built for two" (`decast.py`).

### 1.3 Facts about variables, approximated by rules

Most rewriting rules are conditions on how variables are used, checked on the text: "assigned
exactly once", "read once", "its address never taken", "between the two, nothing writes any name
`EXPR` reads", "no label in the span", "no call, because a callee can write through a pointer this
cannot see" (`untemp.py`, `uncopy.py`, `unaliasvar.py`, `unhoist.py`, `unsave.py`, `unreload.py`,
`namelocal.py`, `resign.py`).  Others need object identity (`objects.py`), layouts
(`structs.py`), or the call graph (`prune_unreachable.py`, `deadcheck.py`).

EDG records the first group as it parses.  Its cross-reference listing marks each reference to a
variable as a use (`U`), a modification (`M`), both (`C`, as `++v`), or the taking of its address
(`A`).  For `untemp.py`'s example, with a third local added:

```c++
#include <stdint.h>
struct Cnt { int32_t cnt; };
void keep(int32_t *p);
void swap_counts(Cnt *v16, Cnt *n251_1)
{
  int32_t v17;
  int32_t v20;
  int32_t v29;
  v17 = v16->cnt;
  n251_1->cnt = v17;
  v20 = n251_1->cnt;
  v16->cnt = v20;
  v29 = 0;
  keep(&v29);
  ++v29;
}
```

`identityTranslator --edg:xref=<file>` writes (columns abridged):

```
v17  D  line 6       v20  U  line 12
v17  M  line 9       v29  D  line 8
v17  U  line 10      v29  M  line 13
v20  D  line 7       v29  A  line 14
v20  M  line 11      v29  C  line 15
```

`v17` and `v20` are written once and read once; `v29`'s address is taken, so the callee may write
it.  These are the preconditions of `untemp.py` and `uncopy.py`, read from the front end instead of
from the text.

## 2. What ROSE provides

What this build of ROSE gives a tool:

* **The EDG 7.0 front end**: GNU, ISO and Visual C++ dialects (the tools' `--msvc`), with GCC's or
  Visual C++'s headers.  It accepts decompiled 32-bit idioms on this 64-bit target: `(int32_t)p` is
  an error for g++ on x86-64, and for EDG warning #767 ("conversion from pointer to smaller
  integer"), which ROSE does not show.  Its options, and how to downgrade the errors that can be
  downgraded, are in [edg-options.md](edg-options.md).
* **ROSE's AST**, made by edg2sage from EDG's IL.  Every expression has a type, a source range and
  the position of its operator.  Implicit conversions are explicit: they are compiler-generated
  `SgCastExp` nodes, such as the conversion of `w` in `w < u`, so a tool can list each of them with
  its source and target types.  Literals keep their spelling, and declarations, labels and `goto`s
  are nodes.
* **Cross-references** (`RoseRefactor::CrossReferences`): each reference to each entity with its
  kind (declaration, definition, use, modification, address taken), and for each entity its kind,
  qualified name, type and scope.  Locals are separate entities, so `v59` in one function is not
  `v59` in another.  Macros and labels are entities too.
* **Source text and edits** (`RoseRefactor::SourceText`, `Edits`): tools change the original text
  at positions from the AST, so formatting, comments and macros stay, and a diff shows only the
  change.  Overlapping edits are reported.
* **Analyses compiled into librose**, not yet tried on edg2sage's AST: call graphs
  (`CallGraphAnalysis`); control flow graphs (`virtualCFG`, `staticCFG`, interprocedural); def-use,
  liveness and SSA (`defUseAnalysis`, `staticSingleAssignment`); read/write sets; dataflow
  frameworks with constant, sign and liveness analyses (`genericDataflow`); pointer analysis
  (`pointerAnal`); constant folding; static slicing; inlining and outlining; AST pattern matching
  (`astMatching`); tree edit distance (`EditDistance`); stable names for AST nodes
  (`abstractHandle`); SARIF output.  It also has struct layout for the i386 and x86-64 ABIs,
  including Visual C++'s variants (`sageInterface/abiStuff.h`), for C structs: it does not lay
  out base classes or virtual functions.
* **Cost.**  On a 36,803-line file shaped like Hex-Rays output (measured in this container):
  parsing with cross-references takes 1.2 s (`rose-ren` listing a name), parsing and building
  ROSE's AST 6 s, and unparsing it 4 s more.  Cross-reference questions can be answered by a
  program that runs once per question; questions about the AST want a process that keeps it (see
  [`rose-serve`](#40-rose-serve-one-parse-many-questions)).

## 3. Conventions for tools that an LLM calls

Most of these the archive learned the hard way; the rest come from what a model can and cannot do
well.  A model is good at judging names and meaning, and at choosing between candidates.  It is
poor at exact counting, at tracking hundreds of locals and their types, at offset arithmetic, and
at keeping thousands of edits consistent.  So the tools should give exact facts and make the
mechanical changes, and leave the judgement to the model and the behaviour to the tests.

* **Ask, then act.**  Every rewriting tool has a query form that lists candidates, each with an
  ID, and the reason each declined candidate is declined.  `--apply <id>` applies one candidate,
  `--apply all` all of them, and `--diff` prints the unified diff without writing.  IDs are
  readable and stable: `function:local` (the form `struct-skip.txt` uses), `Type::member`,
  `file:line:column`.
* **Compact, machine-readable output.**  One fact per line, `--json` for structured output, and a
  last line with the count (`3 candidates, 12 declined`).  Sweeps can then require a zero, and a
  model reads three lines instead of three hundred.  Filters (`--function`, `--name`, `--kind`)
  keep the output in proportion to the question.
* **Facts from the file given, and nothing else.**  No compiler logs, no joins by line number, no
  reading of the working tree.  The output starts with a hash of the input, and an ID from another
  version of the file is refused: `buildlog.py`'s stamp, built in.
* **Edits are text edits at AST positions.**  Formatting, comments and macros stay.  An edit
  inside a macro expansion is refused unless the text is in the macro's arguments, as `rose-ren`
  already does.
* **Every target that matters.**  With `--targets i386,x86_64`, facts that depend on sizes are
  computed for each target, and a rewrite that is exact on one but not on the other is declined:
  `decast.py`'s `(uintptr_t)` cast, a no-op on i386 and the whole point on x86-64.
* **Idempotent and deterministic**, so that a tool can run to a fixpoint ("run it until the count
  stops falling", `uncast.py`).
* **Declining is data.**  The categories of `degoto.py --why` are what the next tool is written
  from.
* **Provenance.**  Each change can be logged as one line (tool, candidate, old text, new text) for
  the commit message.  `addrmap.py` recovered the original address of each function from
  `rename.py`'s output pasted into commit messages.
* **The tests stay in charge.**  The tools prove what syntax and types can prove; the behaviour of
  code that runs off the ends of its arrays is decided by the tests (section 7).

## 4. Query tools

Each of these is read-only.  Most of what the archive's 85 scripts compute by text, before they
change anything, is one of these queries.

### 4.0 `rose-serve`: one parse, many questions

A process that parses a translation unit once, keeps the AST and the cross-references, and
answers the queries below as JSON requests on standard input, without the six seconds that parsing
a file of `subs1.hpp`'s size takes (section 2).  It parses again when the file's hash changes, and
the one-shot tools below are thin clients of the same code.  An agent asks many small questions
between two edits, so this matters more than any single query.

### 4.1 `rose-ask`: what is this?

The entity that a name at a position refers to, its declaration and type; the type of an
expression and of each of its subexpressions, with the implicit conversions applied to them; the
value of a constant expression; and, for a memory access, the address it reaches as its base plus
a byte offset.

The archive computes these by hand.  `negindex.py` takes types from the body's own declarations,
"so a name it cannot type is reported as `?`".  `uncastwidth.py` resolves `img->height` "through
the local's own declared type -- `img->height` is `BmfImage::height` and not whatever else in the
file is called `height`".  `unindex.py` does the arithmetic in its docstring: "`235020 * 4` is
byte 940 080, which is `Obj11::f940072[4]`".  A proposed exchange (this tool does not exist yet;
the member is the archive's, a `uint16_t` array):

```
$ rose-ask subs1.hpp at <line>:<column>
((uint32_t *)v385)[2 * n0x10 + 235020]
  type     uint32_t (lvalue)
  v385     Obj11 *  (local of __alt_p2_model)
  address  (char *)v385 + 940080 + 8 * n0x10
  member   Obj11::f940072 (uint16_t), elements 4 + 4 * n0x10 and 5 + 4 * n0x10
  as       *(uint32_t *)&v385->f940072[4 * n0x10 + 4]
```

Offset arithmetic is exactly where a model is unreliable and a front end is exact; this one query
removes most of the risk of `unindex.py`, `unoffset.py` and `unlane.py`.

### 4.2 `rose-uses`: how is each variable used?

For a variable (local, parameter or global), each reference with its kind and the statement it is
in, grouped by function, and the summaries that the rules test: written once, read once, never
read, address taken, written through (`p->x = ...`), all uses in one block, uses in macros.

* Replaces the text conditions of `untemp.py`, `uncopy.py`, `unaliasvar.py`, `unhoist.py`,
  `unsave.py`, `unreload.py`, `unwrite.py`, `namelocal.py`, and `resign.py`'s "its address is
  never taken"; `extents.py`'s `addr` column; and `unused.py`'s use of `-Wunused-variable`.
* "Between the assignment and the read, nothing writes ..." and "no label in the span" are
  questions about paths, which a control flow graph answers exactly (a label is an entry into the
  span; dominance says whether the read can be reached without the write).
* Each local is its own entity, which is what `rename.py --in FUNC` had to add: "`v59` is a local in
  read_bmp and a different local in thirty other bodies".

### 4.3 `rose-layout`: where is each member, on each target?

The offset, size and alignment of each member of a struct, union or class, the padding, and the
bit-fields; the member (and array element) at a byte offset; whether an access of a given width at
an offset is whole, aligned, narrower or wider than the member; and with `--targets`, the members
whose offsets differ between targets.

* `structs.py` writes a `static_assert` on the size of each struct it recovers, "to say so out
  loud", and `rawoffset.py` reads offsets back out of the `// +N` comments and the
  `__builtin_offsetof(...) == N` assertions.  `unoffset.py`, `unindex.py`, `unlane.py` and
  `unp2.py` each look members up by offset, under the rule "The access type has to match the
  member's": a `uint16_t` store rewritten as a `uint32_t` member "compiles, passes seven of the
  fifteen images, and corrupts the heap".
* EDG lays out each type for the target it parses for (x86-64 Linux, or the win64 target with
  `--msvc`).  ROSE's layout generators compute i386 and x86-64 layouts from the AST.  They do not
  lay out base classes and virtual functions, and they are written for unpacked structs, so
  `#pragma pack` would have to be added (BMF's `BITMAPFILEHEADER` is packed).

### 4.4 `rose-conversions`: every conversion, classified

Every implicit and explicit conversion, with its source and target types and a class: identity,
sign change, narrowing, widening, integer to pointer, pointer to integer (a narrowing on 64-bit
targets), pointer to pointer, the usual arithmetic conversions of a comparison, an integral
promotion.  With each, the position of the operator and the source range of the operand, and per
variable, the conversions into it and out of it.

* This is the information that seven scripts read out of GCC's warnings, without the line-number
  join: `explicitcmp.py`, `resign.py`, `resign_group.py`, `retype_locals.py`, `ptrwidth.py`,
  `decast.py`, `uncast.py` (the eighth, `unused.py`, needs `rose-uses`).  And it includes what GCC
  does not warn about: `unlayer.py`'s chains of pointer casts, `ptrwidth.py`'s pointers laundered
  through `uintptr_t` and its "parked" names
  (`pair_ctx = (uintptr_t)h0; ... (uint32_t)pair_ctx - 1`), and `negindex.py`'s unsigned negated
  index.
* `explicitcmp.py` needs, for `while ( k < pairs )`, the operand that converts and its extent; the
  compiler-generated cast node is exactly that operand, with its range.

### 4.5 `rose-calls`: what calls what, and what is never used

The call graph from given roots, with functions whose address is taken counted as reached;
callers and callees; for each parameter, whether the function reads it and what it passes it on
to; the macros, typedefs, globals and functions that nothing uses.

* `prune_unreachable.py` builds this from identifiers found in bodies ("A function pointer counts
  as a reference").  `deadcheck.py` finds "a body nothing calls", and `undef.py` the unused macros
  of `bmf.cpp`, among them `SDWORD1` to `SDWORD3`: "Three macros that would not compile if anyone
  used them".  `dethread.py` found two `__m128` parameters that fifteen functions and 21
  forwarding shims pass on and none reads.
* EDG's cross-references record each reference to a function, with an `A` where its address is
  taken (`void (*fp)(int32_t *) = keep;`), and the definitions and uses of macros.

### 4.6 `rose-objects`: which names are one object?

Classes of names that denote the same allocation (joined by assignment, argument passing and
return, through forwarding functions) and the field map of each class: the constant byte offsets
it is accessed at, with the widths, and the conflicts.  Also each address as a canonical sum
(base plus a byte coefficient for each variable), which shows one address written two ways.

* `objects.py` builds the classes with a union-find over assignments and parameters it finds in the
  text, and stops at 159 classes; `structs.py` surveys their field maps.  `unspell.py` finds
  "`row[4 * i + 20] = 60` ... `row[2 * i + 10] = 60` ... Those are the same byte" by parsing sums of
  `c * v` terms, and skips "anything with a shift, a call, or a nested subscript".
* With typed expression trees, the byte form of an address is exact: each term is scaled by the
  element size of the pointer it is added to, and constants are folded.

### 4.7 `rose-view`: context for reading

The archive's agent read a 37,000-line file through these scripts.  A model reads better with less
in front of it:

* **A function, annotated**: its locals with their types and use counts, the address-taken ones
  marked, and the Hex-Rays names still in it (`unnamed.py --diff`: "how much of each body is still
  verbatim").
* **What a function needs**: the declarations it depends on (structs, typedefs, globals, the
  signatures of its callees, macros) as one excerpt of the original text, which compiles with the
  function.
* **A slice**: the statements that can affect a variable at a point.  `unhoist.py`'s complaint is
  an expression "two screens further down" from the loads it uses.
* **The documents against the code**: the identifiers that a document names (`unstale.py` reads
  every backticked name in `ALGORITHM.md` and asks whether the source still has it), resolved as
  entities rather than as strings.

### 4.8 `rose-count`: measurements as AST queries

Counts of shapes rather than of spellings: casts by class, dereferences at constant offsets from a
plain pointer (`shape.py`'s three shapes P1, P2 and P3), `goto`s by kind, names in Hex-Rays'
style, stack frames held as buffers.  `--check <document>` compares a table that quotes the counts
with the counts (`checktable.py`).

The archive learned that a count which tests a spelling is defeated by a spelling: `rawoffset.py`
says "An earlier round reported 'raw-offset sites 0' and the count was a spelling", and
`unnamed.py` records four corrections to one row of `shape.py`.  A query over the AST counts the
shape however it is written.

## 5. Rewriting tools

Each of these lists candidates with IDs and reasons, applies one, some or all, and edits the
original text.  The preconditions are the queries of section 4.

### 5.1 `rose-ren`: rename (exists)

`rose-ren` lists the declarations of a name and renames one of them, with all its uses; it is
scope-aware (a local is renamed in its function only), follows qualified names, base classes,
templates and the arguments of macros, and refuses a new name that is already declared in the same
scope.  The archive's `rename.py` also had:

* **Capture by an inner declaration**: "a rename that would collide with a *local* of the same
  name in some body is reported and refused, because the two would become the same identifier".
  `rose-ren` does not check this yet: renaming the global `g` to `h` in
  `int f(void) { int h = 1; g = h; return g; }` makes the function's `g = h` into `h = h`.  The
  check is a lookup of the new name at each reference: any declaration of it that would hide the
  renamed entity there refuses the rename.
* **Renames in batches** from a file, and `function:local` selectors.
* **Renaming part of an identifier** (`rename.py --funcs`): a Hex-Rays function name inside the
  names of the forwarding shims, `__fwd_sub_402FE0_sub_403820`.
* **A log of the renames** for the commit message, which `addrmap.py` relies on.

### 5.2 `rose-retype`: change a declaration, and see what changes meaning

Changes the declared type of a local, parameter, member or global, and first reports each
expression whose meaning would change.  This is what a model most needs to have computed for it,
because a retype that compiles can still change what the program does:

* **Pointer arithmetic scales**: "`p + 1` steps one byte on a `char *` and four on a
  `uint32_t *`" (`retype_locals.py`); `unwiden.py` takes only narrowings: "Widening a cursor would
  make an index step further than it did".
* **Signedness is part of the type**: ordering comparisons whose usual arithmetic conversions
  change, and "the operators whose *meaning* depends on signedness", `>>`, `/` and `%`
  (`resign.py`).  The README's example is a member declared `int32_t` instead of `uint32_t`, "and
  one comparison against `-10`, is four streams".
* **Conversions created and removed**, as a net count, and the other variables that would then
  convert: counting only the conversions into a local "proposed eighteen flips that put the total
  up" (`resign.py`).  With a group option for variables that must agree (`resign_group.py`'s
  connected components) and for registers that hold one quantity (`alti2` to `alti7`).
* **Width**: the reads that use more bits than the new type has (`unloword.py`: "A local whose
  value is *used* at full width keeps its register").
* **Spelling only**: when the new type is the same type under another name (`unify_types.py`'s
  `_DWORD` to `uint32_t`), the report is empty, and that is the proof that "the object file is
  expected to come out bit-identical".

Replaces `retype.py`, `retype_locals.py`, `resign.py`, `resign_group.py`, `unloword.py`,
`unwiden.py`, `unmemcast.py` and `unify_types.py`, and leaves to the model the choice those
scripts leave to their drivers: which candidate to try, with the tests deciding.

### 5.3 `rose-casts`: casts that do nothing, conversions that happen

* **Delete** a cast whose operand already has the target type (`decast.py`, `uncast.py`,
  `unrecast.py`); an inner pointer cast that the outer one overrides (`unlayer.py`); a round trip
  through an integer, `(T *)(int32_t)p` with `p` a pointer, but never `(T *)(int32_t)x` with `x`
  an integer, "the one shape that must never be touched" (`ptrwidth.py --roundtrip`); a store
  through `*(T *)&x.m` whose `T` has the width of `m` and differs only in signedness
  (`uncastwidth.py --same`); and the conservative leftovers of other rewrites
  (`tidy_structs.py`).  Each only when it is a no-op on every target given.
* **Insert** the conversion that an expression already performs, where a reader should see it
  (`explicitcmp.py`'s `k < (uint32_t)pairs`), at the operand the compiler-generated cast marks.

### 5.4 `rose-locals`: temporaries, copies, reloads and saves

Rewrites of locals whose safety is a def-use question:

| Rewrite | Archive script | Condition, as a query |
|---|---|---|
| inline a temporary written once and read once | `untemp.py`, `unhoist.py` | `rose-uses`; the expression has no side effect; nothing on the path between writes what it reads |
| replace a copy of another local by that local | `uncopy.py`, `unaliasvar.py` | both written once (or the source not written while the copy is live); neither address taken |
| keep one of several loads of the same member | `unreload.py`, `uncursor.py` | no write to the member or its object, and no call, between the loads |
| delete a save and its restore around a region | `unsave.py` | the region does not write the saved variable, and no path enters or leaves it in between |
| delete unused locals and stores nobody reads | `unused.py`, `unwrite.py` | no use; the stored expression has no side effect |
| name a local after the member it holds | `namelocal.py` | written once from one member expression; the name is free in the function |
| merge declarations of one type | `compact_locals.py` | no initializer, no attribute, declarators that survive a list |
| split a variable with two lifetimes | (by hand: `unaliasvar.py`'s `sub0_row`) | its definitions and uses fall into separate webs (SSA) |

The last row is new: `unaliasvar.py` leaves "a variable with two lifetimes": "That case has to be
split by hand".  SSA finds the webs, and a rename per web is mechanical.  The control flow
graph replaces the text tests that stand in for paths, such as "the range's brace depth never goes
negative" and "the range contains no label and no `goto`".

### 5.5 `rose-access`: pointer arithmetic to members and subscripts

* **Rewrite accesses** given a pointer's type: `*(T *)((char *)p + N)`, `*((T *)p + K)`,
  `((T *)p)[e + K]` and `p->f278528[i].m128_i32[k]` become the member and subscript at that byte
  offset, when the access covers whole elements of the member at the same width; otherwise the
  access stays, and is counted (`unoffset.py`, `unindex.py`, `unlane.py`, `unp2.py`).
* **Recover a struct** from an object's field map, with padding and a size assertion
  (`structs.py`), and reshape it: runs of one type into arrays (`arrayify.py`, `runarray.py`); a
  struct that is an array with a stride (`unstruct.py`) or a single scalar (`unscalar.py`);
  several recoveries of one object into one struct (`merge.py`), refusing two pointer types at one
  offset ("the dangerous merge"); identical declarations into one (`dedup.py`).
* **Record copies**: the unrolled 4+4+4+4+2-byte moves of an 18-byte record, also with the loads
  hoisted above the stores, into one assignment (`unrec.py`, `unrechoist.py`, `uncopyrec.py`).

`rose-ask`'s address of an access and `rose-layout`'s member at an offset make each of these a
lookup.  Choosing the struct, and its member names, stays with the model.

### 5.6 `rose-flow`: `goto`s, constant conditions, identical branches

* **`goto`s** that have an exact structured form: `if (c) goto L; R L:` to `if (!c) { R }` and its
  `else` variant (`degoto.py`), a forward jump into a block (`unjump.py`), a jump to a short tail
  that ends in `return`, copied to its sources (`untail.py`).  With a control flow graph, "the
  region must be one run of complete statements" and "no label inside the skipped region may be
  reached from outside it" are graph properties.  `degoto.py` learned that brace counts are not:
  "*Balance is not the sum:* a region that closes the enclosing `while` and then opens an `if`
  sums to zero".
* **Constant conditions**: fold an `if` whose condition is constant (`foldif.py`), including
  conditions on globals that are fixed in a set of functions (`deadcheck.py`'s pinned globals),
  given as facts and checked where they are used.
* **Identical branches**: an `if` whose two arms are the same code up to a consistent renaming of
  locals (`undup.py`).  Tree comparison modulo renaming is what `EditDistance` and `astMatching`
  do.  `undup.py` compares token streams, and at first "matched only single-line `if`s", the failure
  that `proven.sh` was written to catch.

### 5.7 `rose-signature`: parameters, receivers, forwarding functions

* **Remove a parameter** and its argument at every call (`dethread.py`, where a forwarding shim is
  "a declaration and a call on one line").  The callers come from the call graph, the arguments
  from the AST.
* **Make a function a member function**: `__f(T *_this, ...)` becomes `T::f(...)`, and its calls
  `x->f(...)` (`methodise.py`).  This is the inverse of `rose-m2g`, which exists and turns a member
  function into a global function with an explicit `This`; the two share the work of finding the
  calls.
* **Inline a forwarding function** whose body is one call with casts (`unshim.py`).

### 5.8 `rose-frame`: stack frames held as byte buffers

Hex-Rays emits some frames as one buffer with a reference for each local:

```c++
alignas(16) uint8_t __hexrays_frame[26712];
char     (&buf)[4096] = *(char (*)[4096])(__hexrays_frame + 0);
int32_t  (&v72)[1024] = *(int32_t (*)[1024])(__hexrays_frame + 4096);
```

The archive turns these into structs with padding (`unframe.py --struct`, `reframe.py`), lifts
members out of them (`defram.py`, `liftframe.py`), gives shared slots and spill areas their own
storage (`unslot.py`, `unspill.py`) and folds the references (`unalias.py`).  The rules are about
what escapes: "An address that escapes does not stop at the member it came from"; "An array member
reaches its neighbours without naming them"; "`&x[i]` is `x + i`".  These are cross-reference and
type questions: an `A` reference, an array that decays to a pointer, a reference bound to a
member.  Whether a frame survives being split is not; that is for the tests (section 7).

### 5.9 `rose-data`: tables, constants and globals

* **Decode a table** stored as bytes and reinterpreted as another type into an initializer of that
  type (`untable.py`: "it reads the image little-endian at the typedef's element width").  The
  front end knows the element type and its size.
* **Rewrite literals** whose value is a pattern (byte fills, IEEE-754 bit patterns, magic numbers)
  in hexadecimal, from the literal's value rather than its text (`hex_constants.py`).
* **Name raw addresses**: an address constant that falls inside a known global becomes that global
  plus an offset (`name_raw_addrs.py`).
* **Globals**: collect, sort and deduplicate declarations (`collect_globals.py`), and give a global
  that is a slice of a larger image its own storage (`deblob.py`, `unbss.py`).  Whether the
  neighbours matter is again a question for the tests: "If it does not, the failure names the
  place where one global's array really does run into the next".

## 6. Checking tools

### 6.1 `rose-same`: did a spelling change change anything?

Compares two versions of a file and says whether they are the same program up to a stated kind of
change: a rename map (`rename.py`, `unalias.py`'s 1,828 sites), type spellings (`unify_types.py`),
deleted identity casts (`uncast.py`), merged declarations (`compact_locals.py`).  Same declarations
with the same types, and the same expression trees with the same types at every node, modulo the
map.  Where the comparison fails, it shows the first difference.

A spelling change checked this way needs no run of the tests at all, and a model's edit by hand
("those two were done by hand", `unsave.py`) can be checked against what it claims to be.

### 6.2 `rose-target`: what means something else at another pointer width

Later rounds made the program work as 64-bit code too, and many of the defects they found were
expressions that are well-formed on both targets and mean different things.  Parsing for i386 and
for x86-64 (or computing both layouts) and comparing gives:

* offsets and sizes that differ, and the constants in the code that equal an offset on one target
  (`rawoffset.py`: "284712 is `offsetof(AltP2Block, p2_ctr)`, thirty times over");
* pointers converted to 32-bit integers, directly, through `uintptr_t`, or through a variable
  (`ptrwidth.py`'s four classes and its "parked" names);
* unsigned values negated and used as indexes (`negindex.py`), and indexes converted to
  `ptrdiff_t`: "`choose_plane_coding` read a union slot as an `int64_t` where the low half was
  wanted ... eight gigabytes of offset with a 64-bit pointer" (`x64diff.sh`).

EDG in this build has the x86-64 and win64 target configurations; full type checking as i386
would need an ILP32 target configuration, while ROSE's layout generators already cover the
layouts.

### 6.3 `rose-probe`: facts that are only known when the program runs

ROSE is a source-to-source compiler, and instrumenting a program is its classic use.  Some of the
archive's questions are about values, not about code:

* **The values a local holds**, which settle its signedness (`resign.py`'s title is "Give a local
  the signedness of the values it actually holds", and it infers that from the conversions).
* **Which functions and branches the tests reach.**  `mkaltp1.py` and `mkmed32.py` were written
  because nothing in the corpus reached `alt_model_p1_decode` or `unpredict_med`, and
  `alt_model_p1_decode` "had been broken since Phase 2 and no test could see it".
* **Which bytes past a member a function touches**: the frames whose neighbours matter,
  measured, rather than found when AddressSanitizer or a segfault reports them (`asan.sh`'s
  `choose_plane_coding`).

The instrumented copy is built and run by the tests; the source is not changed.

## 7. What stays with the tests

The archive's README counts nine rules that its rewriters got wrong first: "six of the nine were
caught by the gate, two by the compiler, and one by the heap allocator.  None by reading."  Tools
built on a front end would have avoided some of them: a rewrite whose result does not type-check
is refused before it is written (the eight compile errors of "a scalar at an array's offset is its
element 0"), and a change of an access's width shows in the types ("a rewrite that keeps the
offset can still change the width").  They would not have avoided the rest: decompiled code reads
and writes memory it does not own ("these bodies walk off the ends of their locals on purpose"),
so whether a frame can be split, a global moved, or two locals separated depends on what the
program does at run time.  The proposals here make the tools' claims exact; the tests still decide
whether the program is the same.

## 8. Where to start

In order of value for the effort, given what `RoseRefactor` already has:

1. **`rose-serve` with `rose-ask`, `rose-uses` and `rose-conversions`.**  These replace the text
   parsing of most of the archive's scripts and all of its compiler-log joins; the information is
   in the AST and the cross-references now.
2. **`rose-casts` and `rose-locals`**: about twenty of the archive's scripts, with preconditions
   that are cross-reference queries.
3. **`rose-retype`** with its report of what changes meaning: the most error-prone change that the
   archive made, done one candidate at a time behind the tests.
4. **`rose-ren`'s capture check** (section 5.1), since `rose-ren` already ships.
5. **`rose-layout` and `rose-access`**: the core of struct recovery; they need the layout
   generators to handle packing.
6. **`rose-same`**, cheap with tree comparison, and the way to check edits by hand.
7. **`rose-signature`** (sharing code with `rose-m2g`), **`rose-flow`**, **`rose-frame`**,
   **`rose-data`**.
8. **`rose-target`** (needs an ILP32 target configuration for full type checking) and
   **`rose-probe`**.

## Appendix: the archive's scripts and the proposed tools

| Proposed tool | Archive scripts it covers |
|---|---|
| `rose-ask` | `structs.py` (types, widths), `negindex.py`, `uncastwidth.py` (census) |
| `rose-uses` | `extents.py`, `unused.py` (query), `unwrite.py` (query) |
| `rose-layout` | `rawoffset.py`, the `static_assert`s of `structs.py` |
| `rose-conversions` | `explicitcmp.py` (query), `ptrwidth.py` (census), `resign.py` (query) |
| `rose-calls` | `prune_unreachable.py` (query), `deadcheck.py`, `undef.py` |
| `rose-objects` | `objects.py`, `unspell.py`, `structs.py` (survey) |
| `rose-view` | `unnamed.py`, `unstale.py` |
| `rose-count` | `shape.py`, `checktable.py` |
| `rose-ren` | `rename.py`, `addrmap.py` (its input) |
| `rose-retype` | `retype.py`, `retype_locals.py`, `resign.py`, `resign_group.py`, `unloword.py`, `unwiden.py`, `unmemcast.py`, `unify_types.py` |
| `rose-casts` | `decast.py`, `uncast.py`, `unrecast.py`, `unlayer.py`, `ptrwidth.py --apply`, `uncastwidth.py --apply`, `tidy_structs.py`, `explicitcmp.py` |
| `rose-locals` | `untemp.py`, `unhoist.py`, `uncopy.py`, `unaliasvar.py`, `unreload.py`, `uncursor.py`, `unsave.py`, `unused.py`, `unwrite.py`, `namelocal.py`, `compact_locals.py` |
| `rose-access` | `structs.py`, `unoffset.py`, `unindex.py`, `unlane.py`, `unp2.py`, `arrayify.py`, `runarray.py`, `unstruct.py`, `unscalar.py`, `merge.py`, `dedup.py`, `unrec.py`, `unrechoist.py`, `uncopyrec.py` |
| `rose-flow` | `degoto.py`, `unjump.py`, `untail.py`, `foldif.py`, `undup.py`, `deadcheck.py` (constant tests) |
| `rose-signature` | `dethread.py`, `methodise.py`, `unshim.py` |
| `rose-frame` | `unframe.py`, `reframe.py`, `defram.py`, `liftframe.py`, `unslot.py`, `unspill.py`, `unalias.py` |
| `rose-data` | `untable.py`, `hex_constants.py`, `name_raw_addrs.py`, `collect_globals.py`, `deblob.py`, `unbss.py` |
| `rose-same` | (new; what `rename.py`, `unify_types.py` and `uncast.py` assume) |
| `rose-target` | `negindex.py`, `ptrwidth.py`, `rawoffset.py`, `x64diff.sh` (the static part) |
| `rose-probe` | (new; what `mkaltp1.py`, `mkmed32.py` and `asan.sh` found by other means) |

Outside ROSE's reach, and rightly so: the tests and their drivers (`test.sh`, `asan.sh`,
`fuzz.sh`, `hdrscan.sh`, `x64.sh`, `x64diff.sh`, `triage.sh`, `mkrefs.sh`, `struct-sweep.sh`,
`frame-sweep.sh`, `resign-drive.sh`, `unmemcast-sweep.sh`), the input generators (`mk*.py`,
`fuzz.py`), and the checks on the tools themselves and on history (`sweep.sh`, `proven.sh`,
`reads.py`, `readme.py`, `outpath.py`, `buildlog.py`, `addrmap.py`).
