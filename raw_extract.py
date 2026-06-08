#!/usr/bin/env python3
# raw_extract.py <beat-file>  ->  stdout raw payload bytes
# Payload = bytes of TLV decompressed-data frames (header type byte == 0x05).
import sys
def raw(path, dt=0x05):
    out = bytearray(); is_data = False; header = False
    for line in open(path, encoding="latin-1"):
        line = line.strip()
        if not line.startswith("0x"):
            continue
        t = line.split(); td = int(t[0], 16); st = 0xff; u = 0
        for x in t[1:]:
            if x.startswith("0x"): st = int(x, 16)
            else:
                if "SoT" in x: u |= 1
                if "EoT" in x: u |= 2
        if u & 1:
            header = True; is_data = ((td & 0xff) == dt)
        if is_data and not header:
            for i in range(bin(st).count("1")):
                out.append((td >> (8 * i)) & 0xff)
        header = False
        if u & 2:
            is_data = False
    return bytes(out)
if __name__ == "__main__":
    sys.stdout.buffer.write(raw(sys.argv[1]))
