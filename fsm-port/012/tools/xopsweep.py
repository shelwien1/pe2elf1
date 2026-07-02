import subprocess
def objd_all(cases):
    # pad each case to 16 bytes with 0x90 (1-byte nop) so objdump can't drift across
    # the block boundary; addr 16*i must always be an instruction boundary.
    blobs=[bytes(c)+b'\x90'*(16-len(c)) for c in cases]
    open('/tmp/big.bin','wb').write(b''.join(blobs))
    out=subprocess.run(['objdump','-D','-b','binary','-m','i386:x86-64','-M','intel','/tmp/big.bin'],capture_output=True,text=True).stdout
    res={}
    for ln in out.splitlines():
        s=ln.strip()
        if ':' in s and '\t' in s:
            try:a=int(s.split(':')[0].strip(),16)
            except:continue
            p=s.split('\t')
            if len(p)>=3:res[a]=p[2].strip()
    return [res.get(16*i,'?MISSING') for i in range(len(cases))]
cases=[];meta=[]
for mapn in (8,9,10):
    for pp in range(4):
        for W in (0,1):
            for L in (0,1):
                b1=(0b111<<5)|mapn        # RXB=111 (no ext) + mmmmm=map
                b2=(W<<7)|(0b1111<<3)|(L<<2)|pp
                for op in range(256):
                    c=[0x8f,b1,b2,op,0xc1,0x00,0x00,0x00,0x00]
                    cases.append(c); meta.append((mapn,pp,W,L,op))
objs=objd_all(cases)
p=subprocess.run(['/tmp/apxbatch'],input='\n'.join(' '.join('%02x'%x for x in c) for c in cases)+'\n',capture_output=True,text=True).stdout.splitlines()
def om(s):
    t=s.split()
    while t and t[0] in ('repz','repnz','data16','lock','rex.W','{bad}','bad'): t=t[1:]
    return t[0] if t else '?'
def mm(s):
    r=s.split('|')[-1].strip(); return r.split()[0] if r.split() else '?'
missing=sum(1 for o in objs if o=='?MISSING')
print("objdump MISSING (alignment drift): %d / %d"%(missing,len(cases)))
gap=[]  # obj clean, mine placeholder/bad
for i,(mapn,pp,W,L,op) in enumerate(meta):
    o=objs[i]; m=p[i] if i<len(p) else '?'
    oclean='(bad)' not in o and o!='?' and o!='?MISSING' and not o.startswith('.byte') and 'bad)' not in o
    myplace = m.startswith('0|') or '(bad)' in m or mm(m)=='vex'
    if oclean and myplace:
        gap.append((mapn,pp,W,L,op,om(o)))
seen={}
for mapn,pp,W,L,op,mn in gap:
    seen.setdefault(mn,[]).append((mapn,pp,W,L,op))
print("XOP placeholder/bad gaps: %d cases, %d distinct mnemonics"%(len(gap),len(seen)))
for mn in sorted(seen):
    e=seen[mn][0]
    print("  %-18s e.g. map%d pp%d W%d L%d op%02x (x%d)"%(mn,e[0],e[1],e[2],e[3],e[4],len(seen[mn])))
