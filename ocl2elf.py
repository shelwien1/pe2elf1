#!/usr/bin/env python3
"""
ocl2elf.py - extract disassemblable ELF objects from an Intel OpenCL CPU
device binary (the blob you get from clGetProgramInfo(CL_PROGRAM_BINARIES),
as produced by the Intel OpenCL CPU runtime / oneAPI DPC++ compiler).

Container layout
----------------
  0x00  48-byte proprietary preamble:
          +0x00  char[6]   magic  "mrpcCL"
          +0x06  uint16    version (1)
          +0x08  byte[32]  opaque build/cache key (NOT a plain digest of the
                           payload - sha256/sha3/blake2s/md5/sha1 all rejected)
          +0x28  uint64    payload size == filesize - 48
  0x30  an ELF64 image whose section table indexes the real content.

The embedded ELF is *almost* standard.  Two deviations break binutils:

  * e_ident[EI_VERSION] and e_version are 0 instead of 1
        -> objcopy/BFD: "file format not recognized"
  * every content section uses sh_type = SHT_NULL instead of SHT_PROGBITS
        -> objcopy/BFD: "the input file has no sections"

(e_type = 0xff04 and sh_addralign = 0 are non-standard too, but binutils and
LLVM both tolerate them.)  All section offsets are relative to the start of
the embedded ELF, i.e. to file offset 0x30 - so once you carve from 0x30 the
image is internally self-consistent.

Sections of interest
--------------------
  .ocl.ir     LLVM bitcode, target triple spir64-unknown-unknown (front-end
              output, before the CPU back-end vectorised anything).
              -> feed to llvm-dis
  .ocl.obj    a fully standard ELF64 x86-64 relocatable object containing the
              native AVX-512 code.  THIS is the file you want to disassemble.
  .ocl.meta   kernel metadata (argument tables, kernel name list)
  .ocl.ver    uint32 format version

Usage
-----
  ocl2elf.py <binary.cl>                     # extract everything next to input
  ocl2elf.py <binary.cl> -o outdir           # choose output directory
  ocl2elf.py <binary.cl> --list              # just show the container layout
  ocl2elf.py <binary.cl> --disasm            # also write an Intel-syntax .asm
"""

import argparse
import os
import struct
import subprocess
import sys

PREAMBLE_MAGIC = b"mrpcCL"

# ELF constants
ELFMAG = b"\x7fELF"
ELFCLASS32, ELFCLASS64 = 1, 2
ELFDATA2LSB, ELFDATA2MSB = 1, 2
EV_CURRENT = 1
SHT_NULL, SHT_PROGBITS, SHT_STRTAB = 0, 1, 3

# Suggested file extension per known Intel OpenCL section name.
SECTION_EXT = {
    ".ocl.ir": ".bc",       # LLVM bitcode
    ".ocl.obj": ".elf",     # native x86-64 relocatable object
    ".ocl.meta": ".bin",
    ".ocl.ver": ".bin",
}


class OclError(Exception):
    pass


def find_elf_base(data, search_limit=4096):
    """Locate the embedded ELF image.

    Prefer the offset advertised by the "mrpcCL" preamble; fall back to
    scanning for the ELF magic so that container variants without the
    preamble (or with a differently sized one) still work.
    """
    if data[:4] == ELFMAG:
        return 0

    if data[:len(PREAMBLE_MAGIC)] == PREAMBLE_MAGIC:
        # Preamble is 48 bytes and records the payload size at +0x28.
        if len(data) >= 0x30:
            size = struct.unpack_from("<Q", data, 0x28)[0]
            if size == len(data) - 0x30 and data[0x30:0x34] == ELFMAG:
                return 0x30

    limit = min(len(data), search_limit)
    off = data.find(ELFMAG, 0, limit)
    while off != -1:
        # e_ident[EI_CLASS] and [EI_DATA] must be sane for this to be a header.
        if off + 6 <= len(data) and data[off + 4] in (ELFCLASS32, ELFCLASS64) \
           and data[off + 5] in (ELFDATA2LSB, ELFDATA2MSB):
            return off
        off = data.find(ELFMAG, off + 1, limit)

    raise OclError("no ELF header found in the first %d bytes" % limit)


def describe_preamble(data):
    """Return a dict describing the 48-byte preamble, or None if absent."""
    if len(data) < 0x30 or data[:len(PREAMBLE_MAGIC)] != PREAMBLE_MAGIC:
        return None
    size = struct.unpack_from("<Q", data, 0x28)[0]
    return {
        "magic": data[:6].decode("ascii"),
        "version": struct.unpack_from("<H", data, 6)[0],
        "key": data[8:0x28].hex(),
        "payload_size": size,
        "payload_size_ok": size == len(data) - 0x30,
    }


class ElfImage(object):
    """Minimal read-only ELF section-table parser.

    `base` is the offset of the ELF header inside `data`; every sh_offset is
    interpreted relative to `base`, which is what makes the embedded image
    self-consistent once carved out.
    """

    def __init__(self, data, base=0):
        if data[base:base + 4] != ELFMAG:
            raise OclError("no ELF magic at offset %#x" % base)
        self.data = data
        self.base = base

        self.elfclass = data[base + 4]
        self.endian = "<" if data[base + 5] == ELFDATA2LSB else ">"
        if self.elfclass not in (ELFCLASS32, ELFCLASS64):
            raise OclError("bad EI_CLASS %d" % self.elfclass)
        self.is64 = self.elfclass == ELFCLASS64

        e = self.endian
        if self.is64:
            fmt = e + "16sHHIQQQIHHHHHH"
        else:
            fmt = e + "16sHHIIIIIHHHHHH"
        (self.e_ident, self.e_type, self.e_machine, self.e_version,
         self.e_entry, self.e_phoff, self.e_shoff, self.e_flags,
         self.e_ehsize, self.e_phentsize, self.e_phnum, self.e_shentsize,
         self.e_shnum, self.e_shstrndx) = struct.unpack_from(fmt, data, base)

        if self.e_shoff == 0 or self.e_shnum == 0:
            raise OclError("embedded ELF has no section table")

        self.sections = [self._read_shdr(i) for i in range(self.e_shnum)]
        self._load_shstrtab()

    def _read_shdr(self, i):
        e = self.endian
        off = self.base + self.e_shoff + i * self.e_shentsize
        if self.is64:
            f = struct.unpack_from(e + "IIQQQQIIQQ", self.data, off)
        else:
            f = struct.unpack_from(e + "IIIIIIIIII", self.data, off)
        keys = ("name", "type", "flags", "addr", "offset", "size",
                "link", "info", "addralign", "entsize")
        sh = dict(zip(keys, f))
        sh["index"] = i
        sh["hdr_offset"] = off
        sh["file_offset"] = self.base + sh["offset"]
        return sh

    def _load_shstrtab(self):
        self.shstrtab = b""
        if self.e_shstrndx == 0 or self.e_shstrndx >= len(self.sections):
            return
        sh = self.sections[self.e_shstrndx]
        self.shstrtab = self.data[sh["file_offset"]:
                                  sh["file_offset"] + sh["size"]]
        for sh in self.sections:
            sh["sname"] = self._str(sh["name"])

    def _str(self, off):
        if off >= len(self.shstrtab):
            return ""
        end = self.shstrtab.find(b"\0", off)
        if end == -1:
            end = len(self.shstrtab)
        return self.shstrtab[off:end].decode("latin1")

    def content(self, sh):
        """Bytes of a section (SHT_NOBITS-like empty sections give b'')."""
        start = sh["file_offset"]
        end = start + sh["size"]
        if end > len(self.data):
            raise OclError("section [%d] %r runs past EOF (%#x > %#x) - "
                           "input looks truncated"
                           % (sh["index"], sh.get("sname", "") or "<unnamed>",
                              end, len(self.data)))
        return self.data[start:end]

    def find(self, name):
        for sh in self.sections:
            if sh.get("sname") == name:
                return sh
        return None


def repair_container(data, base):
    """Carve the embedded image and normalise it so binutils accepts it.

    Only two fixes are load-bearing (verified against binutils objcopy):
      EI_VERSION/e_version 0 -> 1     ("file format not recognized")
      sh_type SHT_NULL -> SHT_PROGBITS ("has no sections")
    sh_addralign 0 -> 1 is cosmetic hygiene; e_type is left alone because
    0xff04 is a legal processor-specific value that both BFD and LLVM accept.
    """
    img = ElfImage(data, base)
    out = bytearray(data[base:])
    e = img.endian
    fixes = []

    if out[6] != EV_CURRENT:
        out[6] = EV_CURRENT
        fixes.append("e_ident[EI_VERSION] 0 -> 1")

    if img.e_version != EV_CURRENT:
        struct.pack_into(e + "I", out, 20, EV_CURRENT)   # e_version, both classes
        fixes.append("e_version %d -> 1" % img.e_version)

    retyped = 0
    realigned = 0
    for sh in img.sections:
        if sh["index"] == 0:
            continue  # SHT_NULL is correct for section 0
        rel = sh["hdr_offset"] - base
        if sh["type"] == SHT_NULL and sh["size"] > 0:
            struct.pack_into(e + "I", out, rel + 4, SHT_PROGBITS)
            retyped += 1
        if sh["addralign"] == 0:
            # Elf64_Shdr: name4 type4 flags8 addr8 offset8 size8 link4 info4
            #             -> sh_addralign at +48.  Elf32_Shdr: all u32 -> +32.
            if img.is64:
                struct.pack_into(e + "Q", out, rel + 48, 1)
            else:
                struct.pack_into(e + "I", out, rel + 32, 1)
            realigned += 1
    if retyped:
        fixes.append("sh_type SHT_NULL -> SHT_PROGBITS on %d section(s)" % retyped)
    if realigned:
        fixes.append("sh_addralign 0 -> 1 on %d section(s)" % realigned)

    return bytes(out), fixes


def human(n):
    return "%#x (%d)" % (n, n)


def cmd_list(data, img, pre):
    print("container: %s" % human(len(data)))
    if pre:
        print("preamble:  magic=%s version=%d payload_size=%s%s"
              % (pre["magic"], pre["version"], human(pre["payload_size"]),
                 "" if pre["payload_size_ok"] else "  *** MISMATCH ***"))
        print("           key=%s" % pre["key"])
    else:
        print("preamble:  none")
    print("embedded ELF at %s: class=ELF%d %s e_type=%#x e_machine=%#x "
          "e_version=%d EI_VERSION=%d shnum=%d"
          % (human(img.base), 64 if img.is64 else 32,
             "LSB" if img.endian == "<" else "MSB",
             img.e_type, img.e_machine, img.e_version, img.e_ident[6],
             img.e_shnum))
    print()
    print("%-3s %-12s %-10s %-12s %-12s %-12s %s"
          % ("idx", "name", "type", "sh_offset", "file_offset", "size", "head"))
    for sh in img.sections:
        head = img.content(sh)[:8].hex(" ") if sh["size"] else ""
        print("%-3d %-12s %-10s %#-12x %#-12x %#-12x %s"
              % (sh["index"], sh.get("sname", "") or "-",
                 {0: "NULL", 1: "PROGBITS", 3: "STRTAB"}.get(sh["type"],
                                                             hex(sh["type"])),
                 sh["offset"], sh["file_offset"], sh["size"], head))


def disassemble(path, out_asm):
    """Try llvm-objdump then GNU objdump; return the tool used or None."""
    attempts = [
        ["llvm-objdump", "-d", "--x86-asm-syntax=intel", "--no-show-raw-insn", path],
        ["objdump", "-d", "-M", "intel", "--no-show-raw-insn", path],
    ]
    for cmd in attempts:
        try:
            res = subprocess.run(cmd, stdout=subprocess.PIPE,
                                 stderr=subprocess.PIPE)
        except FileNotFoundError:
            continue
        if res.returncode == 0 and res.stdout:
            with open(out_asm, "wb") as f:
                f.write(res.stdout)
            return cmd[0]
    return None


def main(argv=None):
    ap = argparse.ArgumentParser(
        description="Extract disassemblable ELF objects from an Intel OpenCL "
                    "CPU device binary.")
    ap.add_argument("input", help="Intel OpenCL device binary (.cl blob)")
    ap.add_argument("-o", "--outdir", default=None,
                    help="output directory (default: alongside the input)")
    ap.add_argument("--list", action="store_true",
                    help="print the container layout and exit")
    ap.add_argument("--disasm", action="store_true",
                    help="also write an Intel-syntax disassembly of .ocl.obj")
    ap.add_argument("--prefix", default=None,
                    help="basename for outputs (default: input stem)")
    args = ap.parse_args(argv)

    with open(args.input, "rb") as f:
        data = f.read()

    base = find_elf_base(data)
    img = ElfImage(data, base)
    pre = describe_preamble(data)

    if args.list:
        cmd_list(data, img, pre)
        return 0

    outdir = args.outdir or (os.path.dirname(os.path.abspath(args.input)))
    if not os.path.isdir(outdir):
        os.makedirs(outdir)
    stem = args.prefix or os.path.splitext(os.path.basename(args.input))[0]

    cmd_list(data, img, pre)
    print()

    written = []

    # 1. The repaired container itself, so binutils can be pointed at it.
    fixed, fixes = repair_container(data, base)
    cpath = os.path.join(outdir, stem + ".container.elf")
    with open(cpath, "wb") as f:
        f.write(fixed)
    written.append((cpath, len(fixed), "repaired container ELF"))
    print("container repairs applied:")
    for fx in fixes:
        print("  - %s" % fx)
    if not fixes:
        print("  - none needed")
    print()

    # 2. Every named section, carved out verbatim.
    obj_path = None
    for sh in img.sections:
        name = sh.get("sname", "")
        if not name or sh["size"] == 0:
            continue
        ext = SECTION_EXT.get(name, ".bin")
        safe = name.lstrip(".").replace(".", "_")
        path = os.path.join(outdir, "%s.%s%s" % (stem, safe, ext))
        blob = img.content(sh)
        with open(path, "wb") as f:
            f.write(blob)
        note = ""
        if blob[:4] == ELFMAG:
            note = "ELF object"
            if name == ".ocl.obj":
                obj_path = path
        elif blob[:4] == b"BC\xc0\xde":
            note = "LLVM bitcode"
        written.append((path, len(blob), note))

    print("written:")
    for path, size, note in written:
        print("  %-52s %10d  %s" % (os.path.basename(path), size, note))

    if args.disasm:
        if obj_path is None:
            print("\n--disasm: no ELF found in .ocl.obj, nothing to do",
                  file=sys.stderr)
        else:
            asm = os.path.splitext(obj_path)[0] + ".asm"
            tool = disassemble(obj_path, asm)
            if tool:
                print("\ndisassembly (%s) -> %s" % (tool, os.path.basename(asm)))
            else:
                print("\n--disasm: neither llvm-objdump nor objdump worked",
                      file=sys.stderr)

    if obj_path:
        print("\nNative code is in %s - disassemble with:"
              % os.path.basename(obj_path))
        print("  objdump -d -M intel %s" % obj_path)
    return 0


if __name__ == "__main__":
    try:
        sys.exit(main())
    except OclError as exc:
        print("error: %s" % exc, file=sys.stderr)
        sys.exit(2)
    except BrokenPipeError:
        # e.g. `ocl2elf.py x --list | head`.  Silence Python's shutdown-time
        # flush of the already-closed stdout.
        try:
            os.dup2(os.open(os.devnull, os.O_WRONLY), sys.stdout.fileno())
        except OSError:
            pass
        sys.exit(0)
