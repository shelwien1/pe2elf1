# Reproducing the x86 / APX instruction reference HTML

This directory holds the tooling that generated `x86/` (Intel SDM vol 2A–2D,
A–Z) and `x86/apx/` (Intel APX, #355828).

There are two independent pipelines:

| Output | Source | Tool | Language |
|--------|--------|------|----------|
| `x86/` (819 pages) | SDM vol 2A–2D | `x86doc/` (fay59, patched) | Python 2.7 + pdfminer 20140328 |
| `x86/apx/` (84 pages) | APX spec #355828 | `apx/apx_extract.py` | Python 3 + pdfminer.six |

## Dependencies

```bash
sudo apt-get install -y qpdf ghostscript poppler-utils
pip3 install pdfminer.six cffi          # cffi fixes a broken system 'cryptography' import
```

`x86doc` is Python 2 only and the original (not `pdfminer.six`) pdfminer — see
"Staging Python 2.7" below. The tooling is run on already-**decrypted** PDFs, so
no PDF-crypto support is needed in either Python.

---

## Part A — SDM (A–Z) with x86doc

### 1. Download the current SDM vol 2 PDFs

These cdrdv2 IDs always redirect to the latest revision (e.g. `253666-092`):

```bash
curl -sL -o vol2a.pdf https://cdrdv2.intel.com/v1/dl/getContent/671199   # 2A  A-L
curl -sL -o vol2b.pdf https://cdrdv2.intel.com/v1/dl/getContent/671241   # 2B  M-U
curl -sL -o vol2c.pdf https://cdrdv2.intel.com/v1/dl/getContent/671072   # 2C  V-Z
curl -sL -o vol2d.pdf https://cdrdv2.intel.com/v1/dl/getContent/671143   # 2D  W-Z + Xeon Phi
```

(The README.upstream.md `dam/.../vol-2a-manual.pdf` URLs still resolve but are
pinned to the **2016** edition — use the cdrdv2 IDs above for current.)

### 2. Decrypt and subset to the instruction pages

The PDFs are encrypted (pdfminer can't read them) and start with front matter.
`qpdf --decrypt` removes the encryption; the page subset implements the README's
"print starting from the first instruction" step. **Page numbers are
revision-specific** — find the first instruction page with `pdftotext` (look for
the first `MNEMONIC—…` heading after the TOC/intro). For revision `-092`:

```bash
for v in vol2a vol2b vol2c vol2d; do qpdf --decrypt $v.pdf $v.dec.pdf; done
qpdf vol2a.dec.pdf --pages vol2a.dec.pdf 119-z  -- vol2a.print.pdf   # from AAA
qpdf vol2b.dec.pdf --pages vol2b.dec.pdf 9-z    -- vol2b.print.pdf   # from MASKMOVDQU
qpdf vol2c.dec.pdf --pages vol2c.dec.pdf 7-z    -- vol2c.print.pdf   # from VADDPH
qpdf vol2d.dec.pdf --pages vol2d.dec.pdf 4-158  -- vol2d.print.pdf   # WAIT/FWAIT..VSCATTERPF (drop appendices)
```

### 3. Stage Python 2.7 (no system Python 2 on Ubuntu 24.04)

Extract the Ubuntu 18.04 python2.7 debs into a prefix (no system install), and
drop the original pdfminer on PYTHONPATH:

```bash
base=http://archive.ubuntu.com/ubuntu/pool/main/p/python2.7
ver=2.7.17-1~18.04ubuntu1.11
mkdir -p ~/py27root
for p in libpython2.7-minimal python2.7-minimal libpython2.7-stdlib libpython2.7 python2.7; do
  curl -sL -o /tmp/$p.deb "$base/${p}_${ver}_amd64.deb"; dpkg-deb -x /tmp/$p.deb ~/py27root
done
curl -sL https://files.pythonhosted.org/packages/57/4f/e1df0437858188d2d36466a7bb89aa024d252bd0b7e3ba90cbc567c6c0b8/pdfminer-20140328.tar.gz | tar -xz -C ~
```

### 4. Run

`extract.py` writes one HTML file per instruction into `html/` (which must exist
and contain `style.css`). Use `-O` (non-debug) so unparseable pages are skipped
instead of aborting — this is the script's intended production mode.

```bash
mkdir -p html && cp /path/to/style.css html/
export PY27_PREFIX=~/py27root PDFMINER_DIR=~/pdfminer-20140328
for v in vol2a vol2b vol2c vol2d; do
  ../py2-run.sh -O extract.py $v.print.pdf      # run each separately for robustness
done
```

Result on `-092`: 2A 235/238, 2B 246/251, 2C 267/271, 2D 56/60 → **819 pages**.

### Parser fixes (`sdm-parser-fixes.patch`)

The 2014 parser aborted on 16 pages of the modern manual. The patched
`x86manual.py`/`htmltext.py` here fix three root causes (see the patch for the
diff vs upstream):

1. **`left_aligned_table`** — snap an item with no exact column match to the
   nearest column by x1 (instead of printing the cells, which also crashed on
   em-dash text under ascii stdout, and aborting).
2. **`__prepare_display`** — bounds-check a trailing lone `•` bullet (latent
   `IndexError`).
3. **`htmltext` `autoclose`/`append`** — tolerate crossed/orphan `sup`/`sub`
   tags instead of aborting.

These recover all 13 real instructions; the only remaining skips are 3
non-instruction chapter-title pages in vol 2D.

---

## Part B — APX (#355828) with apx_extract.py

The APX Architecture Specification is LaTeX-typeset (IntelClear / Computer-Modern
fonts, section-numbered headings) and is **not** in the SDM template, so x86doc
produces nothing from it. `apx/apx_extract.py` is a purpose-built pdfminer.six
extractor: it segments chapters 6–9 by section heading (`N.N MNEMONIC`,
IntelClear-Bold ~14.3pt) and renders each instruction's encoding table,
operand-encoding table, description, OPERATION pseudocode and exceptions.

```bash
curl -sL -o apx.pdf https://cdrdv2.intel.com/v1/dl/getContent/784266   # #355828 APX arch spec
qpdf --decrypt apx.pdf apx.dec.pdf
cp apx_lib.py apx_extract.py .                # both must be importable together
cp /path/to/style.css apx_html/ 2>/dev/null || true
python3 apx_extract.py apx_html               # -> 84 pages
```

`PAGES` in `apx_extract.py` (PDF pages 129–325) covers chapters 6–9 of rev 008;
adjust if the revision changes (the script prints the section count it found).

**Known limitation:** the secondary operand-encoding table has approximate column
splits (the source's multi-word column headers merge under text extraction); all
data is present. The main encoding table and OPERATION blocks are exact.
