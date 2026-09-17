#!/usr/bin/env python3
"""tch2bin.py - an fx2-cmix-transformer checkpoint (.tch) as a weights file.

    python3 tch2bin.py <ckpt.tch> <out.bin>            quantized, like the shipped blobs
    python3 tch2bin.py <ckpt.tch> <out.bin> --fp32     the matrices left as fp32
    ... [--rope ROWS]                                  rope table rows (default 131072)

Reads the torch.save zip directly - no torch, only numpy - and writes the
uncompressed FX2TFW01 container of cpp_infer/SPEC.md section 4, which every
loader in tf/ accepts (the fp32 engine chooses by magic; 3to2 does not read it,
being for the range-coded containers only).

Quantized output applies upstream's own quantizer (pysrc/export_weights.py
quantize_weight_rows): each matrix row divided by the bfloat16-rounded value of
the checkpoint's learned quantize_weight.scale, rounded half to even, clamped
to [-7, 7]. That needs a checkpoint from the quantization-aware stage of
upstream's pipeline (train_6m -> quantize -> cooldown), which carries those
scales; from models/6m-q4-fp32.tch it reproduces models/6m-q4-fp32.tfwc2 with
0 mismatches in 5 868 864 int4 weights, 111 scale tensors, 119 activation
scales and 88 raw tensors.

--fp32 writes the 111 matrices as plain ".weight" fp32 tensors instead. That
is the only way to run a checkpoint from before quantization-aware training
(models/6m.tch has no scales at all), and it needs the fp32 engine
(TF_FP32=1), whose loader takes a plain ".weight" where it finds one.

rope.inv_freq is the 32 exact float bit patterns of the shipped blobs (they are
a property of the architecture, not of a checkpoint). rope.sin/rope.cos are
computed here with numpy's float32 sin/cos, which can differ from the CUDA
tables the range-coded containers regenerate by an ulp in places; a compressor
and its decompressor have to use the same file either way.
"""
import io, pickle, struct, sys, zipfile, collections
import numpy as np

DT_I8, DT_BF16, DT_F32, DT_I32 = 0, 1, 2, 3

ROPE_INV_FREQ_BITS = [
    0x3F800000,0x3F3E32E3,0x3F0D4F93,0x3ED1FA4B,0x3E9C0181,0x3E67D040,0x3E2C3AA4,0x3DFFEB84,
    0x3DBE23AA,0x3D8D4443,0x3D51E97E,0x3D1BF505,0x3CE7BDB3,0x3CAC2CDB,0x3C7FD709,0x3C3E1473,
    0x3C0D38F6,0x3BD1D8AD,0x3B9BE884,0x3B67AB23,0x3B2C1F14,0x3AFFC289,0x3ABE0535,0x3A8D2DA5,
    0x3A51C7E5,0x3A1BDC0C,0x39E79894,0x39AC114A,0x397FAE16,0x393DF603,0x390D2256,0x38D1B717]
CONFIG_INTS = [205, 192, 12, 64, 3, 768, 1024, 64, 3, 4, 10000]   # V D NL DH NH DMLP WIN kDH kNH conv rope_base
CONFIG_KIMI = [1, 1, 1, 0] * 3

# pysrc/export_weights.py's own classification of the state dict
ACTIVATION_SCALE_SUFFIXES = (".quantize_activation.scale", ".quantize_queries.scale",
                             ".quantize_keys.scale", ".quantize_values.scale")
RAW_FP32_SUFFIXES = (".query_convolution.weight", ".key_convolution.weight",
                     ".value_convolution.weight", ".beta_projection.weight", ".dt_bias",
                     ".log_baseline_decay_rate", ".output_fused_norm_gate.weight",
                     ".residual_stream_coefficient.value", ".token_embedding_coefficient.value",
                     "skip_connection_weights.value")


def die(msg):
    sys.stderr.write("tch2bin: " + msg + "\n")
    sys.exit(1)


def load_tch(path):
    """the state dict of a torch.save zip, as numpy arrays, without torch"""
    z = zipfile.ZipFile(path)
    pkl = [n for n in z.namelist() if n.endswith("/data.pkl")]
    if len(pkl) != 1:
        die(f"{path}: not a torch.save archive")
    root = pkl[0][:-len("/data.pkl")]
    np_of = {"FloatStorage": np.float32, "DoubleStorage": np.float64, "HalfStorage": np.float16,
             "BFloat16Storage": np.uint16, "LongStorage": np.int64, "IntStorage": np.int32}

    class Storage:
        def __init__(self, name): self.name = name

    def rebuild(storage, offset, size, stride, *rest):
        return ("T", storage, offset, tuple(size), tuple(stride))

    class Unpickler(pickle.Unpickler):
        def find_class(self, mod, name):
            if (mod, name) == ("collections", "OrderedDict"): return collections.OrderedDict
            if name == "_rebuild_tensor_v2": return rebuild
            if name.endswith("Storage"): return Storage(name)
            die(f"{path}: unexpected object {mod}.{name} in the pickle")
        def persistent_load(self, pid):
            return ("S", pid[1].name, pid[2])       # storage type, key

    sd = Unpickler(io.BytesIO(z.read(root + "/data.pkl"))).load()
    out = collections.OrderedDict()
    for k, v in sd.items():
        if not (isinstance(v, tuple) and v[0] == "T"):
            die(f"{path}: {k} is not a tensor")
        _, (_, stype, key), off, size, stride = v
        expect, s = [], 1
        for d in reversed(size):
            expect.insert(0, s); s *= d
        if tuple(expect) != stride and s > 1:
            die(f"{path}: {k} is not contiguous")
        a = np.frombuffer(z.read(f"{root}/data/{key}"), dtype=np_of[stype])
        out[k] = a[off:off + s].reshape(size)
    return out


def bf16_bits(x):
    """uint16 bfloat16 bits of an fp32 array, round to nearest even"""
    u = np.ascontiguousarray(x, dtype=np.float32).view(np.uint32).astype(np.uint64)
    return (((u + 0x7FFF + ((u >> 16) & 1)) >> 16) & 0xFFFF).astype(np.uint16)


def bf16_to_f32(bits):
    return (bits.astype(np.uint32) << 16).view(np.float32)


def quantize_rows(w, scale_param):
    """pysrc/export_weights.py quantize_weight_rows: per-row scale, fp32
    division, round half to even, clamp to the 15 levels"""
    scale = bf16_to_f32(bf16_bits(scale_param))
    q = np.clip(np.rint(w.astype(np.float32) / scale[:, None]), -7, 7).astype(np.int8)
    return q


def write_tensor_file(path, entries):
    np_of = {DT_I8: np.int8, DT_BF16: np.uint16, DT_F32: np.float32, DT_I32: np.int32}
    with open(path, "wb") as f:
        f.write(b"FX2TFW01")
        f.write(struct.pack("<I", len(entries)))
        for name, dtype, arr in entries:
            data = np.ascontiguousarray(arr, dtype=np_of[dtype])
            enc = name.encode()
            f.write(struct.pack("<I", len(enc)))
            f.write(enc)
            f.write(struct.pack("<B", dtype))
            f.write(struct.pack("<I", data.ndim))
            f.write(struct.pack(f"<{data.ndim}I", *data.shape))
            f.write(data.tobytes())


def main(argv):
    args = [a for a in argv[1:] if not a.startswith("--")]
    flags = [a for a in argv[1:] if a.startswith("--")]
    rope_rows = 131072
    for fl in list(flags):
        if fl.startswith("--rope="):
            rope_rows = int(fl[7:]); flags.remove(fl)
    fp32 = "--fp32" in flags
    if len(args) != 2 or set(flags) - {"--fp32"}:
        sys.stderr.write(__doc__)
        return 1
    src, dst = args

    sd = load_tch(src)
    is_scale = lambda k: k.endswith(".quantize_weight.scale")
    is_act = lambda k: k.endswith(ACTIVATION_SCALE_SUFFIXES)
    is_raw = lambda k: k.endswith(RAW_FP32_SUFFIXES)
    # a matrix is any 2-D ".weight" that is not one of the raw fp32 ones
    is_matrix = lambda k, v: k.endswith(".weight") and v.ndim == 2 and not is_raw(k)

    entries, n_mat, n_act, n_raw = [], 0, 0, 0
    for k, v in sd.items():
        if is_scale(k):
            continue
        if is_matrix(k, v):
            n_mat += 1
            if fp32:
                entries.append((k, DT_F32, v))
                continue
            sk = k[:-len(".weight")] + ".quantize_weight.scale"
            if sk not in sd:
                die(f"{src}: {k} has no quantize_weight.scale - not a checkpoint from the\n"
                    "  quantization-aware stage. Use --fp32 (it then needs the TF_FP32=1 engine).")
            if sd[sk].shape != (v.shape[0],):
                die(f"{src}: {sk} has shape {sd[sk].shape}, expected ({v.shape[0]},)")
            entries.append((k + ".q", DT_I8, quantize_rows(v, sd[sk])))
            entries.append((k + ".scale", DT_BF16, bf16_bits(sd[sk])))
        elif is_act(k):
            n_act += 1
            entries.append((k, DT_BF16, bf16_bits(v)))
        else:
            if not is_raw(k):
                die(f"{src}: do not know what {k} {v.shape} is - not a tensor of this architecture")
            n_raw += 1
            entries.append((k, DT_F32, v))

    if n_mat != 111 or n_raw != 88:
        die(f"{src}: {n_mat} matrices and {n_raw} raw tensors; the architecture has 111 and 88")
    if not fp32 and n_act != 119:
        die(f"{src}: {n_act} activation scales, the quantized engine needs all 119")
    if sd["embedding.weight"].shape != (205, 192):
        die(f"{src}: embedding is {sd['embedding.weight'].shape}, not (205, 192)")

    inv = np.array(ROPE_INV_FREQ_BITS, dtype=np.uint32).view(np.float32)
    t = np.arange(rope_rows, dtype=np.float32)
    freqs = np.outer(t, inv).astype(np.float32)
    entries.append(("rope.inv_freq", DT_F32, inv))
    entries.append(("rope.sin", DT_F32, np.sin(freqs).astype(np.float32)))
    entries.append(("rope.cos", DT_F32, np.cos(freqs).astype(np.float32)))
    entries.append(("config.ints", DT_I32, np.array(CONFIG_INTS, dtype=np.int32)))
    entries.append(("config.kimi", DT_I32, np.array(CONFIG_KIMI, dtype=np.int32)))

    write_tensor_file(dst, entries)
    import os
    sys.stderr.write(f"tch2bin: {src} -> {dst}: {len(entries)} tensors, {os.path.getsize(dst)} bytes, "
                     f"{'fp32 matrices' if fp32 else 'int4 matrices with the checkpoint scales'}, "
                     f"{n_act} activation scales, rope {rope_rows} rows\n")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
