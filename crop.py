import sys,struct
def read_bmp(p):
    d=open(p,'rb').read()
    off=struct.unpack_from('<I',d,10)[0]
    hs=struct.unpack_from('<I',d,14)[0]
    w,h=struct.unpack_from('<ii',d,18)
    planes,bpp=struct.unpack_from('<HH',d,26)
    comp=struct.unpack_from('<I',d,30)[0]
    assert bpp==24 and comp==0,(bpp,comp)
    stride=((w*3+3)//4)*4
    return d,off,w,h,stride,bpp
def crop(src,dst,cw,ch,cx=None,cy=None):
    d,off,w,h,stride,bpp=read_bmp(src)
    H=abs(h)
    cw=min(cw,w); ch=min(ch,H)
    if cx is None: cx=(w-cw)//2
    if cy is None: cy=(H-ch)//2
    ns=((cw*3+3)//4)*4
    rows=[]
    for y in range(ch):
        r=off+(cy+y)*stride+cx*3
        rows.append(d[r:r+cw*3]+b'\0'*(ns-cw*3))
    body=b''.join(rows)
    hdr=bytearray(54)
    hdr[0:2]=b'BM'
    struct.pack_into('<I',hdr,2,54+len(body))
    struct.pack_into('<I',hdr,10,54)
    struct.pack_into('<I',hdr,14,40)
    struct.pack_into('<ii',hdr,18,cw,ch if h>0 else -ch)
    struct.pack_into('<HH',hdr,26,1,24)
    struct.pack_into('<I',hdr,34,len(body))
    open(dst,'wb').write(bytes(hdr)+body)
    print(dst,cw,'x',ch,54+len(body),'bytes')
if __name__=='__main__':
    crop(sys.argv[1],sys.argv[2],int(sys.argv[3]),int(sys.argv[4]))
