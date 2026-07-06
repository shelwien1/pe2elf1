#!/bin/sh
# t_chunks.sh — round-trip test for stegchunks (the sorted_chunks variant).
#
# Partition english.dic into monotone chunks, hide the stegchunks binary in the
# per-chunk line order, recover it, and check both the recovered payload and the
# restored file.
#
#   sorted_chunks english.dic english.txt     : produce the chunk map
#   stegchunks c english.dic mutated.dic ... stegchunks
#         : permute each chunk's lines to embed the stegchunks binary
#   stegchunks d mutated.dic restored.dic ... recovered
#         : recover the payload and restore canonical (monotone) order
#
# The chunk order is a small channel (~31 KB here), so the binary is only
# partially embedded; the recovered payload matches its leading bytes, and the
# restored file must equal english.dic exactly.

set -e

for prog in ./stegchunks ./sorted_chunks; do
  if [ ! -x "$prog" ]; then echo "error: $prog not found — run 'make' first." >&2; exit 1; fi
done

# chunk map
./sorted_chunks english.dic english.txt

# c: embed the stegchunks binary into english.dic's per-chunk line order
./stegchunks c english.dic mutated.dic english.txt stegchunks

# d: recover the payload and restore the canonical file
./stegchunks d mutated.dic restored.dic english.txt payload

# 1) the restored file must be byte-identical to the original
if cmp -s english.dic restored.dic; then
  echo "PASS: restored.dic == english.dic (canonical order fully restored)"
else
  echo "FAIL: restored.dic differs from english.dic" >&2
  cmp english.dic restored.dic || true
  exit 1
fi

# 2) the recovered payload must match the start of the stegchunks binary; its
#    final byte is a partial range-coder byte and cannot be compared.
size=$(wc -c < payload)
echo "payload size = $size bytes"
if cmp -s -n $((size - 1)) payload stegchunks; then
  echo "PASS: first $((size - 1)) bytes of payload match the start of stegchunks"
  echo "      (final byte is a partial range-coder byte and is not compared)"
else
  echo "FAIL: payload does not match the start of stegchunks" >&2
  cmp payload stegchunks || true
  exit 1
fi
