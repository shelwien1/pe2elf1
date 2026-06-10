#!/usr/bin/env python3
# Quantify how much each deflate-header constraint narrows the key search, by
# brute-forcing all 256 candidate low-bytes of k0 for one file of basis.pak and
# counting how many survive each constraint level. This is the analysis behind
# zipcrypt.md S11 (it needs no CBMC/kissat -- it's the ground truth the SAT
# models encode). Ciphertext below is basis.pak's file 0 (crc=19CFF2AC).

K0, K1, K2 = 0xA3E30892, 0xF9185194, 0xEB474B09          # the real key
SALT = [0x96,0x44,0xE6,0xB2,0x44,0xE0,0xEC,0xDF,0x3C,0x8D,0xA9,0x3A]   # 12-byte header
BODY = [0xBC,0x5A,0x9D,0x4B,0x98,0x1B,0xA4,0xC4,0x64,0xD4,0xCF,0xFD]   # first body bytes
M = 134775813

def crc32(crc, b):                       # standard reflected CRC32 single-byte update
    crc ^= b
    for _ in range(8): crc = (crc >> 1) ^ (0xEDB88320 if crc & 1 else 0)
    return crc & 0xFFFFFFFF

def dec_byte(k2):
    t = (k2 & 0xffff) | 2
    return ((t * (t ^ 1)) >> 8) & 0xff

def decrypt_body(u):                      # u = candidate low byte of k0
    k0, k1, k2 = (K0 & ~0xFF) | u, K1, K2
    out = []
    for i, c in enumerate(SALT + BODY):
        pt = c ^ dec_byte(k2)
        if i >= len(SALT): out.append(pt)        # collect body plaintext only
        k0 = crc32(k0, pt)
        k1 = ((k1 + (k0 & 0xff)) * M + 1) & 0xFFFFFFFF
        k2 = crc32(k2, (k1 >> 24) & 0xff)
    return out

def bit(b, p): return (b[p >> 3] >> (p & 7)) & 1
def fld(b, p, n): return sum(bit(b, p + i) << i for i in range(n))
def kraft(b):
    ncode = fld(b, 13, 4) + 4
    return sum(128 >> fld(b, 17 + 3 * j, 3) for j in range(ncode) if fld(b, 17 + 3 * j, 3) > 0)

L0 = [u for u in range(256) if fld(decrypt_body(u), 0, 3) == 5]                 # BFINAL=1|BTYPE=10
L1 = [u for u in L0 if fld(decrypt_body(u), 3, 5) <= 29 and fld(decrypt_body(u), 8, 5) <= 29]
L2 = [u for u in L1 if kraft(decrypt_body(u)) == 128]

print("candidate low-bytes of k0 surviving each constraint level (one file, 256 total):")
print(f"  L0  BFINAL|BTYPE (3 bits)        : {len(L0):3d}")
print(f"  L1  + nlen<=286, ndist<=30       : {len(L1):3d}   (validity ranges -- weak)")
print(f"  L2  + code-length Kraft==128      : {len(L2):3d}   {[hex(x) for x in L2]}   (completeness -- strong)")
print(f"true low byte of k0 = 0x{K0 & 0xFF:02X}")
