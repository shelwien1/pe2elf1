#!/usr/bin/env python3
# Test input for the .p7s path: a two-revision PDF with two signature dictionaries, each /Contents
# a detached CMS signature (openssl, self-signed RSA-2048) over its /ByteRange, 8 KB reserved.
#
#   tests/mksigned.py out.pdf workdir      (needs openssl on PATH; poppler's pdfsig verifies it)
import subprocess, sys, os
out, wd = sys.argv[1], sys.argv[2]
os.makedirs(wd, exist_ok=True)
for k, cn in (('', 'Test Signer'), ('2', 'Second Signer')):
    if not os.path.exists(wd + '/key%s.pem' % k):
        subprocess.run(['openssl', 'req', '-x509', '-newkey', 'rsa:2048', '-nodes', '-keyout', wd + '/key%s.pem' % k,
                        '-out', wd + '/cert%s.pem' % k, '-days', '365', '-subj', '/CN=%s/O=Example' % cn],
                       capture_output=True, check=True)
RES = 8192
def sign(data, key, cert):
    open(wd+'/tbs.bin','wb').write(data)
    return subprocess.run(['openssl','cms','-sign','-binary','-in',wd+'/tbs.bin','-signer',cert,'-inkey',key,
                           '-outform','DER','-nosmimecap'],capture_output=True,check=True).stdout
def build(objs, prev=None, base=b'', size=None):
    buf = bytearray(base); offs = {}
    if not base: buf += b'%PDF-1.7\n%\xe2\xe3\xcf\xd3\n'
    for num, body in objs:
        offs[num] = len(buf); buf += b'%d 0 obj\n' % num + body + b'\nendobj\n'
    xref = len(buf)
    buf += b'xref\n'
    nums = sorted(offs)
    if not base: buf += b'0 1\n0000000000 65535 f \n'
    for n in nums: buf += b'%d 1\n%010d 00000 n \n' % (n, offs[n])
    buf += b'trailer\n<< /Size %d /Root 1 0 R' % (size or max(nums)+1)
    if prev is not None: buf += b' /Prev %d' % prev
    buf += b' >>\nstartxref\n%d\n%%%%EOF\n' % xref
    return buf, xref
content = b'BT /F1 24 Tf 72 720 Td (Signed document) Tj ET'
sigdict = lambda n, name: b'<< /Type /Sig /Filter /Adobe.PPKLite /SubFilter /adbe.pkcs7.detached /Name (' + name + b') /M (D:20260101120000+00\'00\') /ByteRange [0 0000000000 0000000000 0000000000] /Contents <' + b'0'*(2*RES) + b'> >>'
objs = [(1, b'<< /Type /Catalog /Pages 2 0 R /AcroForm << /Fields [6 0 R] /SigFlags 3 >> >>'),
        (2, b'<< /Type /Pages /Kids [3 0 R] /Count 1 >>'),
        (3, b'<< /Type /Page /Parent 2 0 R /MediaBox [0 0 612 792] /Contents 4 0 R /Resources << /Font << /F1 7 0 R >> >> /Annots [6 0 R] >>'),
        (4, b'<< /Length %d >>\nstream\n' % len(content) + content + b'\nendstream'),
        (5, sigdict(5, b'Test Signer')),
        (6, b'<< /Type /Annot /Subtype /Widget /FT /Sig /T (Sig1) /V 5 0 R /Rect [0 0 0 0] /F 132 /P 3 0 R >>'),
        (7, b'<< /Type /Font /Subtype /Type1 /BaseFont /Helvetica >>')]
def fill(buf, key, cert, start_at=0):
    i = buf.index(b'/ByteRange [0 0000000000', start_at)
    c0 = buf.index(b'/Contents <', i) + len(b'/Contents ')
    c1 = buf.index(b'>', c0) + 1
    br = b'/ByteRange [0 %010d %010d %010d]' % (c0, c1, len(buf) - c1)
    buf[i:i+len(br)] = br
    der = sign(bytes(buf[:c0]) + bytes(buf[c1:]), key, cert)
    h = der.hex().upper().encode(); assert len(h) <= 2*RES
    buf[c0+1:c0+1+len(h)] = h
    return c1
buf, x1 = build(objs)
fill(buf, wd+'/key.pem', wd+'/cert.pem')
# incremental update: second signature
objs2 = [(1, b'<< /Type /Catalog /Pages 2 0 R /AcroForm << /Fields [6 0 R 9 0 R] /SigFlags 3 >> >>'),
         (8, sigdict(8, b'Second Signer')),
         (9, b'<< /Type /Annot /Subtype /Widget /FT /Sig /T (Sig2) /V 8 0 R /Rect [0 0 0 0] /F 132 /P 3 0 R >>')]
n1 = len(buf)
buf2, x2 = build(objs2, prev=x1, base=buf, size=10)
fill(buf2, wd+'/key2.pem', wd+'/cert2.pem', start_at=n1)
open(out,'wb').write(buf2)
