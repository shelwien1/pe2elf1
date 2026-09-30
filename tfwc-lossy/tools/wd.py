import numpy as np, struct
DT = {0: np.int8, 1: np.uint16, 2: np.float32, 3: np.int32}
def load(path):
    b = open(path, 'rb').read(); p = 4
    n, = struct.unpack_from('<I', b, p); p += 4
    out = []
    for _ in range(n):
        nl, = struct.unpack_from('<I', b, p); p += 4
        name = b[p:p+nl].decode(); p += nl
        dt = b[p]; p += 1
        nd, = struct.unpack_from('<I', b, p); p += 4
        shape = struct.unpack_from('<%dI' % nd, b, p); p += 4*nd
        nb, = struct.unpack_from('<Q', b, p); p += 8
        data = np.frombuffer(b[p:p+nb], dtype=DT[dt]).copy(); p += nb
        if nb: data = data.reshape(shape)
        out.append([name, dt, tuple(shape), data])
    return out
def save(path, ts):
    o = bytearray(b'WDMP'); o += struct.pack('<I', len(ts))
    for name, dt, shape, data in ts:
        nb = name.encode(); o += struct.pack('<I', len(nb)) + nb + bytes([dt])
        o += struct.pack('<I', len(shape)) + struct.pack('<%dI' % len(shape), *shape)
        raw = b'' if data is None or data.size == 0 else np.ascontiguousarray(data.astype(DT[dt])).tobytes()
        o += struct.pack('<Q', len(raw)) + raw
    open(path, 'wb').write(o)
def bf16(u16):
    return (u16.astype(np.uint32) << 16).view(np.float32)
def to_bf16(f):
    u = np.asarray(f, np.float32).view(np.uint32).astype(np.uint64)
    u = u + 0x7FFF + ((u >> 16) & 1)
    return (u >> 16).astype(np.uint16)
