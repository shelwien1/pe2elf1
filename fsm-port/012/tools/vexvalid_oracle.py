#!/usr/bin/env python3
# Build a VEX/EVEX/XOP opcode-VALIDITY oracle from Intel XED (bundled with SDE).
#
# For every (kind, map, pp, W, opcode) key we probe XED across all 8 ModR/M.reg
# values x {reg-form, [rax] mem-form, VSIB (rm=100+SIB) mem-form, disp32 (rm=101)
# mem-form} x {L=0,1,2} with canonical vvvv/mask. A key is VALID iff XED accepts
# ANY probe (so a key stays decodable if XED defines it in *any* /digit, addressing
# form, or vector length). The VSIB form is essential: gather/scatter (vgatherd*,
# vpgatherd*, vscatter*) are #UD except with a vector-index SIB, so probing only
# reg + [rax] would wrongly mark their whole key invalid. Keys XED rejects across
# every probe are the genuinely-undefined encodings that should decode to #UD
# instead of the structural `vex` placeholder. This maximally-conservative "reject
# only what XED rejects in every form" rule cannot #UD anything XED accepts.
#
# Emits a C header (x86_vexvalid.h): a packed bitmap keyed by
# (kindbits<<..|map<<..|pp<<1|W)*256 + opcode, plus a vexvalid() accessor.
#
#   python3 tools/vexvalid_oracle.py /path/to/xed64 x86_vexvalid.h
import subprocess, sys

XED = sys.argv[1] if len(sys.argv) > 1 else "/tmp/sde-external-10.8.0-2026-03-15-lin/xed64"
OUT = sys.argv[2] if len(sys.argv) > 2 else "x86_vexvalid.h"

# kind ids match x86dec.hpp in->vex: 2 = C4 VEX(3-byte), 4 = 8F XOP, 6 = 62 EVEX.
# (C5 2-byte VEX == C4 map1 W0; the decoder reuses the VEX table for it.)
KINDS = {
    'VEX': (2, [1, 2, 3]),
    'XOP': (4, [8, 9, 10]),
    'EVEX': (6, [1, 2, 3, 4, 5, 6]),
}
# ModR/M "tails" (modrm [+ sib/disp]) covering every addressing form a VEX opcode
# can require: register-direct, plain [rax] memory, VSIB (rm=100 + a vector-index
# SIB, needed by gather/scatter), and rm=101 disp32. One per ModR/M.reg 0..7.
TAILS = []
for r in range(8):
    TAILS.append([0xC0 | (r << 3)])                       # mod=11 reg=r rm=0  (reg-direct)
    TAILS.append([0x00 | (r << 3)])                       # mod=00 reg=r rm=0  ([rax])
    TAILS.append([0x04 | (r << 3), 0x00])                 # mod=00 reg=r rm=100 + SIB (VSIB/[rax+rax])
    TAILS.append([0x05 | (r << 3), 0, 0, 0, 0])           # mod=00 reg=r rm=101 + disp32
PAD = " " + " ".join("90" for _ in range(11))   # trailing bytes for imm/disp completeness

# EVEX P2 aaa (mask select) variants: 0 = no mask (k0), 1 = k1. Gather/scatter are #UD
# without a mask (aaa != 0), so probing aaa=0 only would wrongly mark their key invalid.
EVEX_AAA = (0, 1)

def enc(kind, mp, pp, w, op, tail, L, aaa=0):
    if kind == 'VEX':                           # C4 P1 P2 op tail
        p1 = 0xE0 | mp
        p2 = (w << 7) | 0x78 | (L << 2) | pp     # vvvv=1111
        return [0xC4, p1, p2, op] + tail
    if kind == 'XOP':                            # 8F P1 P2 op tail
        p1 = 0xE0 | mp
        p2 = (w << 7) | 0x78 | (L << 2) | pp
        return [0x8F, p1, p2, op] + tail
    # EVEX: 62 P0 P1 P2 op tail
    p0 = 0xF0 | (mp & 7)                          # R X B R'=1, rsv/bit3=0, mmm=map
    p1 = (w << 7) | 0x78 | 0x04 | pp             # vvvv=1111, bit2=1
    p2 = 0x08 | (L << 5) | (aaa & 7)              # z=0 LL=L b=0 V'=1 aaa
    return [0x62, p0, p1, p2, op] + tail

def classify(line):
    s = line.strip().lower()
    if not s: return False
    if 'udb' in s or 'error' in s or 'too_short' in s or 'too_long' in s:
        return False
    return True

# Build every probe line, remember which key it belongs to.
lines = []
keyprobe = []   # (kind, mp, pp, w, op) per line
for kname, (kid, maps) in KINDS.items():
    for mp in maps:
        for pp in range(4):
            for w in range(2):
                for op in range(256):
                    for L in range(3):
                        for tail in TAILS:
                            aaas = EVEX_AAA if kname == 'EVEX' else (0,)
                            for aaa in aaas:
                                b = enc(kname, mp, pp, w, op, tail, L, aaa)
                                lines.append("X " + " ".join("%02x" % x for x in b) + PAD)
                                keyprobe.append((kname, mp, pp, w, op))

sys.stderr.write("probes: %d\n" % len(lines))
p = subprocess.run([XED, "-64", "-F", "X "], input="\n".join(lines) + "\n",
                   capture_output=True, text=True)
out = p.stdout.splitlines()
if len(out) != len(lines):
    sys.stderr.write("WARN: output lines %d != probes %d\n" % (len(out), len(lines)))

valid = {}   # key -> bool (any probe accepted)
for (key, res) in zip(keyprobe, out):
    ok = classify(res)
    if ok: valid[key] = True
    elif key not in valid: valid[key] = False

# Report
inv = sum(1 for v in valid.values() if not v)
sys.stderr.write("keys: %d, invalid(all-forms-rejected): %d\n" % (len(valid), inv))
for kname, (kid, maps) in KINDS.items():
    for mp in maps:
        tot = sum(1 for k in valid if k[0] == kname and k[1] == mp)
        bad = sum(1 for k, v in valid.items() if k[0] == kname and k[1] == mp and not v)
        sys.stderr.write("  %-4s map%-2d : %3d/%d keys invalid\n" % (kname, mp, bad, tot))

# ---- emit bitmap header ----
# key index layout (must match x86dec.hpp): idx = ((kid2<<3 | map4)<<3 | pp2<<1 | W1)*256 + op
#   kid2: VEX=0, XOP=1, EVEX=2  (compact)   map4: 0..15   pp: 0..3   W: 0..1
KID2 = {'VEX': 0, 'XOP': 1, 'EVEX': 2}
NKEY = 3 * 16 * 4 * 2            # kid2 * map * pp * W
bitmap = bytearray((NKEY * 256 + 7) // 8)   # 1 = VALID (default 0 = invalid/unknown)
def setbit(i):
    bitmap[i >> 3] |= (1 << (i & 7))
for (kname, mp, pp, w, op), v in valid.items():
    if not v: continue
    idx = (((KID2[kname] << 4 | mp) << 3) | (pp << 1) | w) * 256 + op
    setbit(idx)

with open(OUT, "w") as f:
    f.write("// GENERATED by tools/vexvalid_oracle.py from Intel XED -- do not edit.\n")
    f.write("// VEX/EVEX/XOP opcode validity bitmap: 1 = XED decodes this (kind,map,pp,W,opcode)\n")
    f.write("// in some /digit+form+L; 0 = XED rejects it in every form (=> #UD, not the placeholder).\n")
    f.write("// Key idx = (((kid2<<4|map)<<3)|(pp<<1)|W)*256 + opcode; kid2: VEX=0 XOP=1 EVEX=2.\n")
    f.write("#ifndef X86_VEXVALID_H\n#define X86_VEXVALID_H\n")
    f.write("static const unsigned char vexvalid_bits[] = {\n")
    for i in range(0, len(bitmap), 24):
        f.write("  " + ",".join("0x%02x" % b for b in bitmap[i:i+24]) + ",\n")
    f.write("};\n")
    f.write("// kid2: 0=VEX(C4/C5) 1=XOP(8F) 2=EVEX(62). Returns 1 if the key is a defined encoding.\n")
    f.write("static inline int vexvalid(int kid2, int map, int pp, int W, int op) {\n")
    f.write("  if (map < 0 || map > 15) return 0;\n")
    f.write("  unsigned idx = (((unsigned)(kid2<<4|map)<<3)|((pp&3)<<1)|(W&1))*256u + (unsigned)(op&0xff);\n")
    f.write("  return (vexvalid_bits[idx>>3] >> (idx&7)) & 1;\n")
    f.write("}\n#endif\n")
sys.stderr.write("wrote %s (%d bytes bitmap)\n" % (OUT, len(bitmap)))
