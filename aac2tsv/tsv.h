/*
 * aac2tsv - TSV records: output records (c mode) and a streaming record
 * reader (d mode, and the per-frame verification of c mode).
 *
 * A record is one line: a name, then tab separated fields. Fields are
 * decimal integers, hex strings (bytes or nibbles), binary strings (bit
 * vectors) or "-" for an empty string. Lines starting with '#' are comments.
 *
 * Licensed under the GNU Lesser General Public License v2.1 or later.
 */

/* TSV input error (d mode): bad or inconsistent record */
struct TsvError {
    size_t line;
    std::string msg;
};

enum RecName {
    R_HDR, R_RAW, R_EOF,
    R_ADTSH, R_ADTS, R_CRC, R_RDBPOS, R_TAIL, R_REST,
    R_ADIF, R_ADIFPCE, R_RAWASC, R_AUS1, R_AU, R_AUTAIL,
    R_LOAS, R_SMC, R_ASC, R_SMCX, R_PAY, R_PAYRAW, R_PAYPAD, R_OTHER,
    R_SCE, R_CPE, R_CCE, R_LFE, R_DSE, R_PCE, R_FIL, R_END,
    R_ICS, R_MS, R_SEC, R_SF, R_PULSE, R_TNS, R_GC, R_SPEC, R_CGAIN,
    R_FILL, R_EXT, R_DRC, R_SBR, R_SBRC,
    R_SBRH, R_SBRD, R_SGRID, R_SDTDF, R_SINVF, R_SENV, R_SNOISE, R_SHARM,
    R_SBRX, R_SBRXE, R_SBRPAD, R_SBRINIT,
    R_PS, R_PSIID, R_PSICC, R_PSX, R_PSXE, R_PSIPD, R_PSPD,
    R_COUNT
};

static const char *const rec_names[R_COUNT] = {
    "aac2tsv", "raw", "eof",
    "adtsh", "adts", "crc", "rdbpos", "tail", "rest",
    "adif", "adifpce", "rawasc", "aus1", "au", "autail",
    "loas", "smc", "asc", "smcx", "pay", "payraw", "paypad", "other",
    "sce", "cpe", "cce", "lfe", "dse", "pce", "fil", "end",
    "ics", "ms", "sec", "sf", "pulse", "tns", "gc", "spec", "cgain",
    "fill", "ext", "drc", "sbr", "sbrc",
    "sbrh", "sbrd", "sgrid", "sdtdf", "sinvf", "senv", "snoise", "sharm",
    "sbrx", "sbrxe", "sbrpad", "sbrinit",
    "ps", "psiid", "psicc", "psx", "psxe", "psipd", "pspd",
};

static int rec_lookup(const char *s, size_t n)
{
    for (int i = 0; i < R_COUNT; i++)
        if (strlen(rec_names[i]) == n && !memcmp(rec_names[i], s, n))
            return i;
    return -1;
}

static const char hexdig[] = "0123456789abcdef";

static int hexval(int c)
{
    return c >= '0' && c <= '9' ? c - '0' : c >= 'a' && c <= 'f' ? c - 'a' + 10 :
           c >= 'A' && c <= 'F' ? c - 'A' + 10 : -1;
}

static void append_int(std::string &s, int64_t v)
{
    char t[24];
    int n = 0;
    uint64_t u = v < 0 ? 0 - (uint64_t)v : (uint64_t)v;
    do {
        t[n++] = (char)('0' + u % 10);
        u /= 10;
    } while (u);
    if (v < 0)
        t[n++] = '-';
    while (n)
        s += t[--n];
}

/* ---------------- output (c mode) ---------------- */

struct OutRec {
    int name;
    bool omit;
    std::string s;  /* "\tfield\tfield..." */

    void i(int64_t v)
    {
        s += '\t';
        append_int(s, v);
    }
    void str(const char *p, size_t n)
    {
        s += '\t';
        if (!n)
            s += '-';
        else
            s.append(p, n);
    }
    /* nbits (multiple of 4) of buf from bit from, as hex nibbles */
    void hex(const uint8_t *buf, size_t from, size_t nbits)
    {
        s += '\t';
        if (!nbits) {
            s += '-';
            return;
        }
        for (size_t k = 0; k < nbits; k += 4) {
            unsigned v = 0;
            for (int j = 0; j < 4; j++)
                v = v << 1 | (k + j < nbits ? bit_at(buf, from + k + j) : 0);
            s += hexdig[v];
        }
    }
    void hexbytes(const uint8_t *p, size_t n)
    {
        s += '\t';
        if (!n) {
            s += '-';
            return;
        }
        for (size_t k = 0; k < n; k++) {
            s += hexdig[p[k] >> 4];
            s += hexdig[p[k] & 15];
        }
    }
    /* bit vector, first bit first */
    void bits(const uint8_t *v, int n)
    {
        s += '\t';
        if (!n)
            s += '-';
        for (int k = 0; k < n; k++)
            s += (char)('0' + v[k]);
    }
};

struct OutFrame {
    std::deque<OutRec> recs;   /* deque: references stay valid while adding */
    size_t n = 0;

    void clear() { n = 0; }
    OutRec &add(int name)
    {
        if (n == recs.size())
            recs.emplace_back();
        OutRec &r = recs[n++];
        r.name = name;
        r.omit = false;
        r.s.clear();
        return r;
    }
    void format(std::string &out) const
    {
        for (size_t k = 0; k < n; k++) {
            const OutRec &r = recs[k];
            if (r.omit)
                continue;
            out += rec_names[r.name];
            out += r.s;
            out += '\n';
        }
    }
};

/* ---------------- input (d mode) ---------------- */

/* lines from a FILE or from memory; '\r' before '\n' is dropped */
struct LineSource {
    FILE *f = nullptr;
    const char *mem = nullptr;
    size_t memlen = 0, mempos = 0;
    std::vector<char> buf;
    size_t bpos = 0, blen = 0;
    bool feof_ = false;
    size_t lineno = 0;

    void open_file(FILE *fp)
    {
        f = fp;
        buf.resize(1 << 20);
        bpos = blen = 0;
        feof_ = false;
        lineno = 0;
    }
    void open_mem(const char *p, size_t n)
    {
        f = nullptr;
        mem = p;
        memlen = n;
        mempos = 0;
        lineno = 0;
    }
    bool getline(std::string &out)
    {
        out.clear();
        if (!f) {
            if (mempos >= memlen)
                return false;
            const char *p = mem + mempos;
            const char *e = (const char *)memchr(p, '\n', memlen - mempos);
            size_t n = e ? (size_t)(e - p) : memlen - mempos;
            out.assign(p, n);
            mempos += n + (e ? 1 : 0);
        } else {
            bool any = false;
            for (;;) {
                if (bpos == blen) {
                    if (feof_)
                        break;
                    blen = fread(buf.data(), 1, buf.size(), f);
                    bpos = 0;
                    if (!blen) {
                        feof_ = true;
                        break;
                    }
                }
                any = true;
                const char *p = buf.data() + bpos;
                const char *e = (const char *)memchr(p, '\n', blen - bpos);
                if (e) {
                    out.append(p, e - p);
                    bpos += (e - p) + 1;
                    break;
                }
                out.append(p, blen - bpos);
                bpos = blen;
            }
            if (!any)
                return false;
        }
        if (!out.empty() && out.back() == '\r')
            out.pop_back();
        lineno++;
        return true;
    }
};

struct InRec {
    int name = -1;
    size_t line = 0;
    std::string text;
    std::vector<uint32_t> fs, fe;  /* field spans in text */

    size_t nf() const { return fs.size(); }
};

struct RecStream {
    LineSource src;
    InRec rec[2];
    int cur = 0;
    bool have_look = false, at_end = false;
    InRec dummy;

    bool fetch(InRec &r)
    {
        for (;;) {
            if (!src.getline(r.text))
                return false;
            if (r.text.empty() || r.text[0] == '#')
                continue;
            r.line = src.lineno;
            r.fs.clear();
            r.fe.clear();
            const char *t = r.text.data();
            size_t n = r.text.size(), p = 0;
            const char *tab = (const char *)memchr(t, '\t', n);
            size_t ne = tab ? (size_t)(tab - t) : n;
            r.name = rec_lookup(t, ne);
            if (r.name < 0)
                throw TsvError{ r.line, "unknown record '" + r.text.substr(0, ne) + "'" };
            p = ne;
            while (p < n) {
                p++; /* tab */
                const char *q = (const char *)memchr(t + p, '\t', n - p);
                size_t e = q ? (size_t)(q - t) : n;
                if (e == p)
                    throw TsvError{ r.line, "empty field" };
                r.fs.push_back((uint32_t)p);
                r.fe.push_back((uint32_t)e);
                p = e;
            }
            return true;
        }
    }
    /* name of the next record, -1 at the end */
    int peek()
    {
        if (!have_look) {
            if (at_end)
                return -1;
            if (!fetch(rec[cur ^ 1])) {
                at_end = true;
                return -1;
            }
            have_look = true;
        }
        return rec[cur ^ 1].name;
    }
    /* consume the next record; nullptr at the end */
    InRec *next()
    {
        if (peek() < 0)
            return nullptr;
        cur ^= 1;
        have_look = false;
        return &rec[cur];
    }
    size_t line() const { return rec[cur].line; }
};

static int64_t parse_int(const InRec &r, size_t k)
{
    const char *p = r.text.data() + r.fs[k], *e = r.text.data() + r.fe[k];
    bool neg = false;
    if (p < e && *p == '-') {
        neg = true;
        p++;
    }
    if (p == e || e - p > 18)
        throw TsvError{ r.line, "bad integer field " + std::to_string(k + 1) };
    int64_t v = 0;
    for (; p < e; p++) {
        if (*p < '0' || *p > '9')
            throw TsvError{ r.line, "bad integer field " + std::to_string(k + 1) };
        v = v * 10 + (*p - '0');
    }
    return neg ? -v : v;
}
