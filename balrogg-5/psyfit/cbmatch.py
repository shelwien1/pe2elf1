import subprocess, os, sys
def recs(p):
    for line in open(p).read().split("\n")[1:]:
        if not line: continue
        t,v=line.split("\t")
        for x in v.split(" "): yield (t,int(x))
def lk1(ent,dim):
    v=1
    while True:
        a=1
        for _ in range(dim): a*=v
        if a>ent: return v-1
        v+=1
def parse(p):
    it=recs(p); books=[]
    for t,v in it:
        if t=='cb.count': n=v+1; break
    for _ in range(n):
        dim=next(it)[1]; ent=next(it)[1]; ordered=next(it)[1]
        L=[]
        if ordered:
            cur=next(it)[1]; got=0
            while got<ent:
                r=next(it)[1]; L+=[cur]*r; got+=r; cur+=1
        else:
            sparse=next(it)[1]
            if not sparse: L=[next(it)[1]+1 for _ in range(ent)]
            else:
                u=next(it)[1]
                for i in range(ent):
                    if i: u=next(it)[1]
                    L.append(next(it)[1]+1 if u else 0)
        look=next(it)[1]; ql=()
        if look:
            f=tuple(next(it)[1] for _ in range(6)); vb=next(it)[1]; seq=next(it)[1]
            nv=lk1(ent,dim) if look==1 else ent*dim
            ql=tuple(next(it)[1] for _ in range(nv))
        h=0
        for x in L: h=(h*131+x)&((1<<64)-1)
        hq=0
        for x in ql: hq=(hq*131+x)&((1<<64)-1)
        books.append((dim,ent,look,h,hq))
    return books
def hdr(p):
    it=recs(p); d={}
    for t,v in it:
        if t in ('id.channels','id.rate'): d[t]=v
        if len(d)==2: return d['id.channels'], d['id.rate']
    return None,None
def libv(ch,rate,q):
    out=subprocess.run(['/tmp/bk2',str(ch),str(rate),str(q)],capture_output=True,text=True).stdout
    bs=[]
    for line in out.split("\n"):
        if not line.startswith('B '): continue
        f=line.split(); mt=int(f[5])
        bs.append((int(f[1]),int(f[2]),mt,int(f[4][1:])&((1<<64)-1),
                   (int(f[-1][1:])&((1<<64)-1)) if mt else 0))
    return bs
files=sorted('testfiles/'+f for f in os.listdir('testfiles') if f.endswith('.ogg'))
files+=sorted('gen/'+f for f in os.listdir('gen') if f.endswith('.ogg'))
hit=0
for f in files:
    subprocess.run(['./balrogg/balrogg','c',f,'/tmp/z.tsv'],check=True)
    s=parse('/tmp/z.tsv'); ch,rate=hdr('/tmp/z.tsv')
    best=None
    for qi in range(-1,11):
        l=libv(ch,rate,qi*0.1)
        if len(l)==len(s) and all(a==b for a,b in zip(s,l)): best=qi*0.1; break
    if best is not None: hit+=1
    print("  %-14s %dch %6dHz  %2d books  %s" % (os.path.basename(f),ch,rate,len(s),
          ("MATCH at q=%.1f" % best) if best is not None else "no match"))
print("\n  %d of %d files reproduce their entire codebook set from libvorbis" % (hit,len(files)))
