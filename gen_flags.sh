#!/usr/bin/env bash
# Translate a Project-Zipline VCS .vlist into a Verilator -f flag file.
# Usage: ./gen_flags.sh <RTL_ROOT> <path/to/foo.vlist> <out.f> <top_source.v>
# The .vlist supplies the +incdir/-y search closure; we drop its explicit top
# (absolute path ending in .v) and append the chosen top source instead.
set -euo pipefail
RTL_ROOT="$1"; VLIST="$2"; OUT="$3"; TOPSRC="$4"
iconv -f ISO-8859-1 -t UTF-8 "$VLIST" | tr -d '\r' \
  | sed "s:\\\$RTL_ROOT:${RTL_ROOT}:g" \
  | grep -vE '^\s*($|/\*|\*|\*/|//)' \
  | grep -vE '^/.*\.v$' > "$OUT"        # drop only the explicit absolute top path
echo "+libext+.svp" >> "$OUT"           # packages live in *.svp; let -y resolve them
echo "$TOPSRC" >> "$OUT"
