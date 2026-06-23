# -*- coding: utf-8 -*-
"""Shared helpers for extracting APX instruction pages from #355828 (decrypted)."""
import re
from pdfminer.high_level import extract_pages
from pdfminer.layout import LTTextContainer, LTChar, LTTextLineHorizontal

PAGE_H = 792.0  # letter

def page_lines(pdf, page_numbers=None):
    """Yield (page_index, [frag,...]) where frag = dict(x0,x1,ytop,font,size,text).
    ytop is a global top-down coordinate (page_index*PAGE_H + (PAGE_H - y0))."""
    for page in extract_pages(pdf, page_numbers=page_numbers):
        pi = page.pageid  # 1-based-ish; we instead track via enumerate outside
        frags = []
        def walk(o):
            for c in o:
                if isinstance(c, LTTextLineHorizontal):
                    chars = [ch for ch in c if isinstance(ch, LTChar)]
                    if not chars:
                        continue
                    txt = c.get_text().rstrip("\n")
                    if not txt.strip():
                        continue
                    # dominant font/size = most common among chars
                    font = chars[0].fontname.split('+')[-1]
                    size = round(max(ch.size for ch in chars), 1)
                    frags.append(dict(x0=round(c.x0, 1), x1=round(c.x1, 1),
                                      y0=round(c.y0, 1), font=font, size=size,
                                      text=txt))
                elif isinstance(c, LTTextContainer):
                    walk(c)
        walk(page)
        yield frags

HEAD_RE = re.compile(r'^(\d+)\.(\d+)\s+([A-Za-z][A-Za-z0-9/]*)\s*$')
SUB_RE  = re.compile(r'^(\d+)\.(\d+)\.(\d+)\s+(.+?)\s*$')

def is_section_head(f):
    return f['font'].startswith('IntelClear-Bold') and f['size'] >= 13 and HEAD_RE.match(f['text'])

def is_subsection_head(f):
    return f['font'].startswith('IntelClear-Bold') and 11.0 <= f['size'] < 13.5 and SUB_RE.match(f['text'])
