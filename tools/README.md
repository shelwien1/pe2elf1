# Tooling

Generators for the instruction-reference HTML in this repo. Full instructions
in [REPRODUCE.md](REPRODUCE.md).

- `x86doc/` — [fay59/x86doc](https://github.com/fay59/x86doc) (public domain),
  patched to parse the current Intel SDM. Produces `x86/` from SDM vol 2A–2D.
  Python 2.7 + pdfminer 20140328. See `sdm-parser-fixes.patch` for the changes.
- `apx/` — purpose-built pdfminer.six extractor for the Intel APX spec
  (#355828), whose LaTeX typesetting x86doc cannot read. Produces `x86/apx/`.
  Python 3.
- `py2-run.sh` — wrapper that runs the Python-2 scripts against a locally-staged
  Python 2.7 (Ubuntu 24.04 ships none).
