#!/usr/bin/env python3
# make_noncanon.py -- build noncanon.bin, a corpus of NON-CANONICAL and
# NON-STANDARD x86-32 encodings that still decode to the same x86insn_t as their
# shorter/standard twin. Every instruction here is one the corpus.p tables model,
# chosen so the whole file decodes; the point is that  asm32 c (asm32 d X)  must
# reproduce X byte-for-byte even though a naive "canonical" assembler would emit
# something shorter. Run:  python3 make_noncanon.py && <roundtrip via asm32>.

cases = [
    # ---- non-canonical DISPLACEMENT WIDTH (same effective address) ----
    ("8b 00",                "mov eax,[eax]                 canonical baseline (no disp)"),
    ("8b 40 00",             "mov eax,[eax+0]               redundant disp8 = 0"),
    ("8b 80 00 00 00 00",    "mov eax,[eax+0]               redundant disp32 = 0"),
    ("8b 80 05 00 00 00",    "mov eax,[eax+5]               disp32 where disp8 would do"),
    ("8b 45 00",             "mov eax,[ebp+0]               [ebp] (disp8=0 is forced, canonical)"),
    ("8b 85 00 00 00 00",    "mov eax,[ebp+0]               [ebp] widened to disp32"),

    # ---- gratuitous / dead-field SIB (same effective address) ----
    ("8b 04 20",             "mov eax,[eax]                 gratuitous SIB, no index"),
    ("8b 04 60",             "mov eax,[eax]                 gratuitous SIB, DEAD scale=*2"),
    ("8b 04 e0",             "mov eax,[eax]                 gratuitous SIB, DEAD scale=*8"),
    ("8b 04 25 00 00 00 00", "mov eax,[0]                   [disp32] via SIB no-base (vs 8b 05)"),
    ("8d 04 4d 00 00 00 00", "lea eax,[ecx*2+0]             SIB index, no base, disp32=0"),

    # ---- 16-bit addressing (67), incl. non-canonical disp ----
    ("67 8b 00",             "mov eax,[bx+si]               16-bit addressing"),
    ("67 8b 46 00",          "mov eax,[bp+0]                16-bit [bp], disp8=0"),
    ("67 8b 06 00 00",       "mov eax,[0]                   16-bit [disp16]"),

    # ---- redundant / ineffective PREFIXES (replayed verbatim) ----
    ("3e 8b 00",             "mov eax,ds:[eax]              redundant DS override (DS is default)"),
    ("26 8b 00",             "mov eax,es:[eax]              ES override"),
    ("2e 8b 45 00",          "mov eax,cs:[ebp+0]            CS override + redundant disp8"),
    ("f0 01 00",             "lock add [eax],eax            LOCK prefix"),
    ("66 66 b8 34 12",       "mov ax,0x1234                 duplicated 66 operand-size prefix"),
    ("2e 66 66 66 66 66 66 66 66 66 66 66 66 66 a4",
                             "cs: movsb                     14-byte prefix run (CS + 13x 66)"),

    # ---- opcode-choice TWINS (same operands, different opcode -> enc witness) ----
    ("01 c8",                "add eax,ecx                   store form (01)"),
    ("03 c1",                "add eax,ecx                   load form  (03) -- same operands"),
    ("89 d8",                "mov eax,ebx                   store form (89)"),
    ("8b c3",                "mov eax,ebx                   load form  (8b) -- same operands"),
    ("83 c0 10",             "add eax,0x10                  imm8 sign-extended (83 /0)"),
    ("81 c0 10 00 00 00",    "add eax,0x10                  same value as imm32 (81 /0)"),
    ("40",                   "inc eax                       short form (40+r)"),
    ("ff c0",                "inc eax                       group form (FF /0) -- same operand"),
]


def main():
    blob = bytearray()
    print("offset  bytes                              meaning")
    print("-" * 78)
    for hexs, desc in cases:
        b = bytes(int(x, 16) for x in hexs.split())
        print("%5d   %-34s %s" % (len(blob), hexs, desc))
        blob += b
    with open("noncanon.bin", "wb") as f:
        f.write(blob)
    print("-" * 78)
    print("wrote noncanon.bin: %d instructions, %d bytes" % (len(cases), len(blob)))


if __name__ == "__main__":
    main()
