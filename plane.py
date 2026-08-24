import sys,struct,numpy as np
def load(p):
    d=open(p,'rb').read(); off=struct.unpack_from('<I',d,10)[0]
    w,h=struct.unpack_from('<ii',d,18); bpp=struct.unpack_from('<H',d,28)[0]
    assert bpp==24; H=abs(h); stride=((w*3+3)//4)*4
    a=np.frombuffer(d[off:off+stride*H],dtype=np.uint8).reshape(H,stride)[:,:w*3].reshape(H,w,3)
    return a,w,H,h>0
src,dst,comp=sys.argv[1],sys.argv[2],int(sys.argv[3])
a,w,H,bottomup=load(src)
ch=np.ascontiguousarray(a[:,:,comp])
stride=((w+3)//4)*4
rows=[ch[y].tobytes()+b'\0'*(stride-w) for y in range(H)]
body=b''.join(rows)
pal=b''.join(bytes([i,i,i,0]) for i in range(256))
hdr=bytearray(54)
hdr[0:2]=b'BM'
struct.pack_into('<I',hdr,2,54+len(pal)+len(body))
struct.pack_into('<I',hdr,10,54+len(pal))
struct.pack_into('<I',hdr,14,40)
struct.pack_into('<ii',hdr,18,w,H if bottomup else -H)
struct.pack_into('<HH',hdr,26,1,8)
struct.pack_into('<I',hdr,34,len(body))
struct.pack_into('<II',hdr,46,256,256)
open(dst,'wb').write(bytes(hdr)+pal+body)
print(dst,w,'x',H,'8bpp grey',54+len(pal)+len(body),'bytes')
