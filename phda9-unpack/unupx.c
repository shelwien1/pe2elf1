/*
 * unupx - static unpacker for ELF64/amd64 executables packed with the
 *         modified UPX 3.94 variant used by the phda9 releases.
 *
 * The packer is UPX (l_info magic "UPX!", format 22 = linux/elf64-amd64,
 * method 14 = LZMA), but its LZMA stream is not stock:
 *
 *   - the two property bytes are  b0 = (lc+lp)<<3 | pb , b1 = lp<<4 | lc
 *     instead of the usual single byte (pb*5+lp)*9+lc
 *   - lc is allowed up to 8 (liblzma caps it at 4)
 *   - the range coder is initialised with range = 0xFFFF0404 instead of
 *     0xFFFFFFFF
 *
 * so neither `upx -d` nor liblzma can decode it.  Rather than reimplement
 * that decoder, this tool reuses the decompressor that ships inside the
 * packed file itself: the UPX entry point is
 *
 *     e_entry:  call  stub_main        <- f_expand() is the return address
 *
 * so f_expand() lives at e_entry+5 and follows the SysV convention
 *
 *     int f_expand(const u8 *src, size_t src_len,
 *                  u8 *dst, unsigned *dst_len, int method);
 *
 * The whole packed image is mapped RWX at a scratch address and that
 * function is called directly.  The CALL/JMP unfilter (UPX filter 0x49)
 * is reimplemented here; it is position independent, so its output can be
 * checked against a memory dump of the real stub's work.
 *
 * Build:  gcc -O2 -o unupx unupx.c
 * Usage:  ./unupx <packed> <unpacked>
 */

#define _GNU_SOURCE
#include <elf.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>

#define SCRATCH ((void *) 0x50000000UL)

struct l_info {                 /* UPX loader info, follows the phdrs */
    uint32_t l_checksum;
    uint32_t l_magic;           /* "UPX!" */
    uint16_t l_lsize;
    uint8_t  l_version;
    uint8_t  l_format;
};

struct p_info {                 /* original program info */
    uint32_t p_progid;
    uint32_t p_filesize;
    uint32_t p_blocksize;
};

struct b_info {                 /* one compressed block */
    uint32_t sz_unc;
    uint32_t sz_cpr;
    uint8_t  b_method;
    uint8_t  b_ftid;
    uint8_t  b_cto8;
    uint8_t  b_unused;
};

typedef int (*expand_fn)(const uint8_t *src, size_t src_len,
                         uint8_t *dst, unsigned *dst_len, int method);

static expand_fn f_expand;

static void die(const char *msg) { fprintf(stderr, "unupx: %s\n", msg); exit(1); }

/*
 * UPX filter 0x49, inverse transform, transcribed from the stub's
 * f_unfilter().  Marked E8/E9/0F8x operands hold  cto8 || 24-bit big
 * endian target; restore the little endian rel32.  Only the offset of the
 * operand inside the buffer is used, so this is position independent.
 */
static void unfilter49(uint8_t *buf, size_t len, uint8_t cto)
{
    size_t i = 0;
    uint8_t op;

    if (len < 6)
        return;
    i = 5;                      /* the stub consumes 4+1 bytes up front */
    op = buf[4];
    for (;;) {
        int marked = 0;
        if (i >= len)
            break;
        if (op >= 0x80 && op <= 0x8f && i >= 2 && buf[i - 2] == 0x0f)
            marked = 1;         /* two byte jcc */
        else if ((uint8_t)(op - 0xe8) <= 1)
            marked = 1;         /* call rel32 / jmp rel32 */

        if (marked && i + 4 <= len && buf[i] == cto) {
            uint32_t v = ((uint32_t) buf[i + 1] << 16) |
                         ((uint32_t) buf[i + 2] << 8) |
                          (uint32_t) buf[i + 3];
            v -= (uint32_t) i;
            buf[i]     = (uint8_t)  v;
            buf[i + 1] = (uint8_t) (v >> 8);
            buf[i + 2] = (uint8_t) (v >> 16);
            buf[i + 3] = (uint8_t) (v >> 24);
            i += 4;
            if (i >= len)
                break;
            op = buf[i++];      /* skipped over: not rescanned for 0f 8x */
            if (i >= len)
                break;
            continue;
        }
        op = buf[i++];
    }
}

static uint8_t *unpack_block(const uint8_t *img, size_t off,
                             const struct b_info *b, const char *what)
{
    unsigned dst_len = b->sz_unc;
    uint8_t *dst = malloc(b->sz_unc + 64);

    if (!dst)
        die("out of memory");
    if (b->sz_cpr == b->sz_unc) {               /* stored verbatim */
        memcpy(dst, img + off, b->sz_unc);
    } else {
        if (b->b_method != 14)
            die("unsupported compression method");
        if (f_expand(img + off, b->sz_cpr, dst, &dst_len, b->b_method) != 0 ||
            dst_len != b->sz_unc)
            die("f_expand failed");
    }
    if (b->b_ftid) {
        if (b->b_ftid != 0x49)
            die("unsupported filter id");
        unfilter49(dst, b->sz_unc, b->b_cto8);
    }
    fprintf(stderr, "  %-22s %8u -> %8u  method %2u filter %#04x/%#04x\n",
            what, b->sz_cpr, b->sz_unc, b->b_method, b->b_ftid, b->b_cto8);
    return dst;
}

int main(int argc, char **argv)
{
    FILE *f;
    long fsz;
    uint8_t *img, *out;
    Elf64_Ehdr *eh;
    Elf64_Phdr *ph;
    struct l_info *li;
    struct p_info *pi;
    size_t off, hdr_end, i;
    unsigned outsz;
    /* file ranges of the original, in ascending order, and what fills them */
    struct { size_t beg, end; } load[64];
    int nload = 0;

    if (argc != 3) {
        fprintf(stderr, "usage: %s <packed> <unpacked>\n", argv[0]);
        return 2;
    }
    if (!(f = fopen(argv[1], "rb")))
        die("cannot open input");
    fseek(f, 0, SEEK_END);
    fsz = ftell(f);
    rewind(f);
    img = malloc(fsz);
    if (fread(img, 1, fsz, f) != (size_t) fsz)
        die("short read");
    fclose(f);

    eh = (Elf64_Ehdr *) img;
    if (memcmp(eh->e_ident, ELFMAG, 4) || eh->e_ident[EI_CLASS] != ELFCLASS64)
        die("not an ELF64 file");
    ph = (Elf64_Phdr *) (img + eh->e_phoff);

    li = (struct l_info *) (img + eh->e_phoff + eh->e_phnum * eh->e_phentsize);
    if (memcmp(&li->l_magic, "UPX!", 4))
        die("no UPX l_info found after the program headers");
    pi = (struct p_info *) (li + 1);
    fprintf(stderr, "UPX version %u format %u loader %u bytes, original %u bytes\n",
            li->l_version, li->l_format, li->l_lsize, pi->p_filesize);

    /* map the packed image so the stub's f_expand() can be called */
    if (mmap(SCRATCH, (fsz + 0xffff) & ~0xffffUL,
             PROT_READ | PROT_WRITE | PROT_EXEC,
             MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED, -1, 0) != SCRATCH)
        die("cannot map scratch image");
    memcpy(SCRATCH, img, fsz);
    /* e_entry is  "call stub_main"; the pushed return address is f_expand */
    f_expand = (expand_fn) ((uint8_t *) SCRATCH +
                            (eh->e_entry - ph[0].p_vaddr) + 5);

    outsz = pi->p_filesize;
    out = calloc(1, outsz);
    if (!out)
        die("out of memory");

    /* ---- block 0: the original Ehdr + Phdrs ---------------------------- */
    off = (uint8_t *) (pi + 1) - img;
    {
        struct b_info *b = (struct b_info *) (img + off);
        uint8_t *d = unpack_block(img, off + sizeof *b, b, "ehdr+phdrs");
        memcpy(out, d, b->sz_unc);
        hdr_end = b->sz_unc;
        off += sizeof *b + b->sz_cpr;
        free(d);
    }

    {   /* collect the original PT_LOAD file ranges, ascending by offset */
        Elf64_Ehdr *oeh = (Elf64_Ehdr *) out;
        Elf64_Phdr *oph = (Elf64_Phdr *) (out + oeh->e_phoff);
        fprintf(stderr, "original entry %#lx, %u program headers\n",
                (unsigned long) oeh->e_entry, oeh->e_phnum);
        for (i = 0; i < oeh->e_phnum; i++)
            if (oph[i].p_type == PT_LOAD && oph[i].p_filesz) {
                if (nload == 64)
                    die("too many PT_LOADs");
                load[nload].beg = oph[i].p_offset;
                load[nload].end = oph[i].p_offset + oph[i].p_filesz;
                nload++;
            }
        for (i = 1; i < (size_t) nload; i++)
            if (load[i].beg < load[i - 1].beg)
                die("PT_LOADs are not sorted by file offset");
        if (!nload || load[0].beg != 0 || load[0].end < hdr_end)
            die("unexpected first PT_LOAD");
    }

    /* ---- the block chain: the PT_LOAD contents ------------------------- */
    {
        int li_idx = 0;
        size_t pos = hdr_end;
        while (li_idx < nload) {
            struct b_info *b;
            uint8_t *d;
            if (pos >= load[li_idx].end) {
                if (++li_idx == nload)
                    break;
                pos = load[li_idx].beg;
                continue;
            }
            b = (struct b_info *) (img + off);
            if (pos + b->sz_unc > load[li_idx].end)
                die("block overruns its PT_LOAD");
            d = unpack_block(img, off + sizeof *b, b, "PT_LOAD contents");
            memcpy(out + pos, d, b->sz_unc);
            free(d);
            pos += b->sz_unc;
            off += sizeof *b + (b->sz_cpr < b->sz_unc ? b->sz_cpr : b->sz_unc);
        }
    }

    /*
     * What is left are the parts of the original file that no PT_LOAD
     * covers: inter-segment alignment padding and the trailing section
     * header table.  UPX stores those as further b_info blocks placed
     * behind the loader, in ascending file order.  The loader is not
     * placed at a fixed distance from the end of the chain, so the blocks
     * are located by scanning for a header matching the gap to be filled.
     */
    {
        size_t pos = 0, scan = off;
        int li_idx = 0;
        while (pos < outsz) {
            struct b_info *b = NULL;
            uint8_t *d;
            size_t hole_end, p;
            if (li_idx < nload && pos >= load[li_idx].beg && pos < load[li_idx].end) {
                pos = load[li_idx].end;
                li_idx++;
                continue;
            }
            hole_end = li_idx < nload ? load[li_idx].beg : outsz;
            if (hole_end <= pos)
                break;
            for (p = scan; p + sizeof *b < (size_t) fsz; p++) {
                struct b_info *c = (struct b_info *) (img + p);
                if (c->sz_unc == hole_end - pos && c->sz_cpr && c->b_unused == 0 &&
                    c->sz_cpr <= c->sz_unc && c->b_method == 14 &&
                    p + sizeof *c + c->sz_cpr <= (size_t) fsz) {
                    b = c;
                    break;
                }
            }
            if (!b)
                die("cannot find the block for a non-loaded gap");
            d = unpack_block(img, p + sizeof *b, b, "non-loaded gap");
            memcpy(out + pos, d, b->sz_unc);
            free(d);
            scan = p + sizeof *b + b->sz_cpr;
            pos = hole_end;
        }
    }

    if (!(f = fopen(argv[2], "wb")))
        die("cannot open output");
    if (fwrite(out, 1, outsz, f) != outsz)
        die("short write");
    fclose(f);
    fprintf(stderr, "wrote %s (%u bytes)\n", argv[2], outsz);
    return 0;
}
