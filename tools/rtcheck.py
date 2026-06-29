#!/usr/bin/env python3
# rtcheck.py -- round-trip / cross-validation harness for corpus64.p.
#
#   tools/rtcheck.py FILE.s            assemble FILE.s with GNU as, then for each
#                                      instruction check  as-bytes -> mydisasm ->
#                                      myasm == as-bytes  and show objdump intel.
#   tools/rtcheck.py --bin FILE.bin    same, but start from a raw code blob.
#
# The byte-exact loop proves my disassembler and assembler are mutually inverse
# AND agree with a production assembler on real encodings; the objdump column is
# for eyeballing that the mnemonics/operands are semantically right.
import sys, os, subprocess, tempfile, re
sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
import parsergen as pg
import asm as asmmod

DESC = os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))), "corpus64.p")


def as_assemble(spath):
  with tempfile.TemporaryDirectory() as d:
    o = os.path.join(d, "a.o")
    b = os.path.join(d, "a.bin")
    subprocess.run(["as", "--64", spath, "-o", o], check=True)
    subprocess.run(["objcopy", "-O", "binary", "--only-section=.text", o, b], check=True)
    objd = subprocess.run(["objdump", "-d", "-M", "intel", o],
                          capture_output=True, text=True).stdout
    return open(b, "rb").read(), objd


def objdump_map(objd):
  # offset -> intel text
  out = {}
  for ln in objd.splitlines():
    m = re.match(r"\s+([0-9a-f]+):\s+((?:[0-9a-f]{2} )+)\s*(.*)", ln)
    if m:
      out[int(m.group(1), 16)] = m.group(3).strip()
  return out


def main(argv):
  if argv and argv[0] == "--bin":
    buf = open(argv[1], "rb").read()
    omap = {}
  else:
    buf, objd = as_assemble(argv[0])
    omap = objdump_map(objd)
  g = pg.Parser(open(DESC).read()).parse()
  a = asmmod.Asm(g)
  pos = 0
  nok = nbad = 0
  while pos < len(buf):
    text, n = asmmod.disasm_one(g, buf[pos:], pos)
    if text is None or n == 0:
      print("  UNDECODABLE @0x%x: %02x" % (pos, buf[pos]))
      nbad += 1
      break
    asbytes = buf[pos:pos + n]
    try:
      myb = asmmod.asm_line(a, text, pos)
      rt = (myb == asbytes)
    except Exception as exc:
      myb = b""
      rt = False
      text += "   <asm FAIL: %s>" % exc
    tag = "ok " if rt else "XX "
    if rt:
      nok += 1
    else:
      nbad += 1
    obj = omap.get(pos, "")
    extra = "" if rt else "  asm=" + myb.hex()
    note = ""
    if obj and not obj.split() == []:
      note = "   ;; objdump: " + obj
    print("%s@%04x %-14s %-34s%s%s" % (tag, pos, asbytes.hex(), text, extra, note))
    pos += n
  print("---- %d ok, %d bad ----" % (nok, nbad))
  return 1 if nbad else 0


if __name__ == "__main__":
  sys.exit(main(sys.argv[1:]))
