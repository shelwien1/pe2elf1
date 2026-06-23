# -*- coding: utf-8 -*-
"""Extract Intel APX (#355828) instruction-reference sections to x86doc-style HTML.

The APX spec is LaTeX-typeset (IntelClear / Computer-Modern fonts) and is NOT in
the SDM template the x86doc parser expects, so this is a purpose-built extractor.
It segments chapters 6-9 by section heading (IntelClear-Bold ~14.3 "N.N MNEMONIC")
and renders the encoding table, operand-encoding table, OPERATION pseudocode and
prose subsections.  Works on the decrypted PDF.
"""
import re, os, sys, html
import apx_lib as A

PDF = 'apx.dec.pdf'
OUTDIR = sys.argv[1] if len(sys.argv) > 1 else 'apx_html'
PAGES = list(range(128, 325))   # 0-based; chapters 6-9 (PDF pages 129-325)

def esc(s):
    return html.escape(s, quote=False)

def collect():
    """All fragments in the instruction chapters, each tagged with a global
    top-down coordinate gy so multi-page sections stay in reading order."""
    out = []
    for n, (pi) in enumerate(PAGES):
        for frags in [next_frags(pi)]:
            for f in frags:
                if f['y0'] >= 660 or f['y0'] <= 52:   # drop running header / footer
                    continue
                f['gy'] = pi * 1000 + (792.0 - f['y0'])
                f['page'] = pi
                out.append(f)
    out.sort(key=lambda f: f['gy'])
    return out

_cache = {}
def next_frags(pi):
    if pi not in _cache:
        _cache[pi] = list(A.page_lines(PDF, page_numbers=[pi]))[0]
    return _cache[pi]

ENC_RE = re.compile(r'^(EVEX|REX2|VEX|MVEX|LEGACY|NP|REX|0F|66|F2|F3)\b')
def is_enc(t):
    return bool(ENC_RE.match(t.strip())) or 'MAP' in t

def nearest(x, cols):
    return min(range(len(cols)), key=lambda i: abs(x - cols[i]))

# ---------- encoding table (Encoding / Instruction / Op-En / Mode / CPUID) ----------
MODE_RE = re.compile(r'^[VIN](/[VINE.]|\b)')
def render_encoding_table(frags):
    headers = [f for f in frags if f['font'].startswith('IntelClear-Bold') and 8 <= f['size'] < 10.5]
    if not headers:
        return ''
    header_bottom = min(f['y0'] for f in headers)
    body = [f for f in frags if f['y0'] < header_bottom - 1 and not f['font'].startswith('IntelClear-Bold')
            and f['size'] >= 8]
    body.sort(key=lambda f: f['gy'])
    # derive column x-positions from the data (robust to merged/odd headers)
    import statistics as st
    enc_xs, openc_xs, mode_xs, cpuid_xs = [], [], [], []
    for f in body:
        t = f['text'].strip()
        if t.startswith('APX') or t in ('CET',):       cpuid_xs.append(f['x0'])
        elif MODE_RE.match(t) and len(t) <= 8:          mode_xs.append(f['x0'])
        elif re.match(r'^[A-Z]$', t):                   openc_xs.append(f['x0'])
        elif is_enc(t):                                 enc_xs.append(f['x0'])
    med = lambda xs, d: st.median(xs) if xs else d
    cols = [med(enc_xs, 78.0), med(openc_xs, None), med(mode_xs, None), med(cpuid_xs, None)]
    if cols[1] is None or cols[2] is None:
        return ''            # not a recognizable encoding table
    if cols[3] is None:
        cols[3] = cols[2] + 60.0
    rows = []
    cur = None
    def flush():
        if cur and (cur['enc'] or cur['instr']):
            rows.append(cur)
    for f in body:
        ci = nearest(f['x0'], cols)
        t = f['text'].strip()
        if ci == 0 and is_enc(t):
            flush(); cur = {'enc': [], 'instr': [], 'openc': [], 'mode': [], 'cpuid': []}
            cur['enc'].append(t)
        else:
            if cur is None:
                cur = {'enc': [], 'instr': [], 'openc': [], 'mode': [], 'cpuid': []}
            cur[['enc', 'openc', 'mode', 'cpuid'][ci] if ci else 'instr'].append(t)
    flush()
    if not rows:
        return ''
    out = ['<table>', '<tr><th>Encoding</th><th>Instruction</th><th>Op/En</th>'
           '<th>64/32-bit Mode</th><th>CPUID Feature Flag</th></tr>']
    for r in rows:
        cells = [' '.join(r['enc']), ' '.join(r['instr']), ' '.join(r['openc']),
                 ' '.join(r['mode']), ' '.join(r['cpuid'])]
        out.append('<tr>' + ''.join('<td>%s</td>' % esc(c) for c in cells) + '</tr>')
    out.append('</table>')
    return '\n'.join(out)

# ---------- generic small table (operand encoding) ----------
def render_grid(frags):
    bold = [f for f in frags if f['font'].startswith('IntelClear-Bold')]
    if not bold:
        return ''
    head_y = max(f['y0'] for f in bold)
    headcells = sorted([f for f in bold if abs(f['y0'] - head_y) < 4], key=lambda f: f['x0'])
    cols = [f['x0'] for f in headcells]
    if len(cols) < 2:
        return ''
    labels = [f['text'].strip() for f in headcells]
    body = [f for f in frags if f['y0'] < head_y - 4]
    # cluster body into rows by y
    body.sort(key=lambda f: -f['y0'])
    rows = []; cur = []; lasty = None
    for f in body:
        if lasty is not None and abs(f['y0'] - lasty) > 6:
            rows.append(cur); cur = []
        cur.append(f); lasty = f['y0']
    if cur: rows.append(cur)
    out = ['<table>', '<tr>' + ''.join('<th>%s</th>' % esc(l) for l in labels) + '</tr>']
    for rcells in rows:
        vals = [''] * len(cols)
        for f in sorted(rcells, key=lambda f: f['x0']):
            vals[nearest(f['x0'], cols)] = (vals[nearest(f['x0'], cols)] + ' ' + f['text'].strip()).strip()
        if any(vals):
            out.append('<tr>' + ''.join('<td>%s</td>' % esc(v) for v in vals) + '</tr>')
    out.append('</table>')
    return '\n'.join(out)

# ---------- OPERATION pseudocode ----------
def render_operation(frags):
    # the pseudocode is set in a mono font (LMMono / Computer-Modern); the tiny
    # line numbers in the left margin are a different font and get dropped here
    code = [f for f in frags if 'Mono' in f['font'] or 'CM' in f['font']]
    if not code:
        return ''
    code.sort(key=lambda f: f['gy'])
    base = min(f['x0'] for f in code)
    lines = []
    for f in code:
        indent = int(round((f['x0'] - base) / 5.0))
        lines.append(' ' * max(0, indent) + f['text'].rstrip())
    return '<pre>' + esc('\n'.join(lines)) + '</pre>'

# ---------- prose ----------
def render_prose(frags):
    items = [f for f in frags if not f['font'].startswith('IntelClear-Bold')
             and 'Mono' not in f['font'] and 'CM' not in f['font'] and f['size'] >= 7.5]
    items.sort(key=lambda f: f['gy'])
    out = []; para = []; lastgy = None
    def flush():
        if para:
            out.append('<p>%s</p>' % esc(' '.join(para)))
    for f in items:
        t = f['text'].strip()
        if not t: continue
        if lastgy is not None and f['gy'] - lastgy > 16:
            flush(); para = []
        if t[0] in u'•−▪' or t.startswith('•'):
            flush(); para = []
            out.append('<p class="bullet">%s</p>' % esc(t.lstrip(u'•−▪• ').strip()))
        else:
            para.append(t)
        lastgy = f['gy']
    flush()
    return '\n'.join(out)

def main():
    if not os.path.isdir(OUTDIR):
        os.makedirs(OUTDIR)
    frags = collect()
    heads = [(i, f) for i, f in enumerate(frags) if A.is_section_head(f)]
    print('sections:', len(heads))
    count = 0
    for k, (i, hf) in enumerate(heads):
        end = heads[k + 1][0] if k + 1 < len(heads) else len(frags)
        body = frags[i + 1:end]
        m = A.HEAD_RE.match(hf['text'].strip())
        mnem = m.group(3)
        # subsection split
        subs = [(j, f) for j, f in enumerate(body) if A.is_subsection_head(f)]
        enc_region = body[:subs[0][0]] if subs else body
        parts = ['<h1>%s</h1>' % esc(mnem)]
        et = render_encoding_table(enc_region)
        if et: parts.append(et)
        for s in range(len(subs)):
            j, sf = subs[s]
            jend = subs[s + 1][0] if s + 1 < len(subs) else len(body)
            seg = body[j + 1:jend]
            title = A.SUB_RE.match(sf['text'].strip()).group(4).strip().title()
            low = title.lower()
            parts.append('<h2>%s</h2>' % esc(title))
            if 'operand encoding' in low:
                parts.append(render_grid(seg) or render_prose(seg))
            elif 'operation' in low:
                parts.append(render_operation(seg) or render_prose(seg))
            else:
                parts.append(render_prose(seg))
        write(mnem, '\n'.join(p for p in parts if p))
        count += 1
    print('wrote', count, 'pages to', OUTDIR)

def write(mnem, bodyhtml):
    fname = mnem.replace('/', ':') + '.html'
    doc = (u'<!DOCTYPE html>\n<html>\n<head>\n<meta charset="UTF-8">\n'
           u'<link rel="stylesheet" type="text/css" href="style.css">\n'
           u'<title>%s (Intel APX)</title>\n</head>\n<body>\n%s\n</body>\n</html>\n'
           % (esc(mnem), bodyhtml))
    with open(os.path.join(OUTDIR, fname), 'w') as fd:
        fd.write(doc)

if __name__ == '__main__':
    main()
