import subprocess, re
def objd_all(cases):
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
for mapn in (1,2,3):
    P0=0xf0|mapn
    for pp in range(4):
        for W in (0,1):
            P1=(W<<7)|0x7c|pp
            for op in range(256):
                for md in (0xc1,0x00):
                    c=[0x62,P0,P1,0x48,op,md]
                    if mapn==3: c.append(0x00)
                    cases.append(c); meta.append((mapn,pp,W,op,md))
objs=objd_all(cases)
p=subprocess.run(['/tmp/apxbatch'],input='\n'.join(' '.join('%02x'%x for x in c) for c in cases)+'\n',capture_output=True,text=True).stdout.splitlines()
def om(s):
    t=s.split()
    while t and t[0] in ('repz','repnz','data16','lock'): t=t[1:]
    return t[0] if t else '?'
def mm(s):
    r=s.split('|')[-1].strip(); return r.split()[0] if r.split() else '?'
# a mnemonic is a genuine gap only if mine NEVER decodes it (at any tested W/md) while objdump does
obj_mnems=set(); mine_mnems=set(); gap_ex={}
for i,(mapn,pp,W,op,md) in enumerate(meta):
    o=objs[i]; m=p[i] if i<len(p) else '?'
    oclean=o!='?MISSING' and '(bad)' not in o and o!='?' and not o.startswith('.byte') and 'bad)' not in o and '{' not in o
    if oclean: obj_mnems.add(om(o))
    if not (m.startswith('0|') or '(bad)' in m or mm(m) in ('vex','evex','?')): mine_mnems.add(mm(m))
    if oclean and (m.startswith('0|') or '(bad)' in m or mm(m) in ('vex','evex','?')):
        gap_ex.setdefault(om(o),(mapn,pp,W,op,md))
missing=sum(1 for o in objs if o=='?MISSING')
print("objdump MISSING (alignment drift): %d / %d"%(missing,len(cases)))
genuine=sorted(m for m in obj_mnems if m not in mine_mnems)
print("EVEX: objdump mnemonics=%d, mine-covered=%d, genuinely-missing=%d"%(len(obj_mnems),len(mine_mnems),len(genuine)))
for mn in genuine[:80]:
    e=gap_ex.get(mn); print("  %-18s %s"%(mn, ('map%d pp%d W%d op%02x'%(e[0],e[1],e[2],e[3])) if e else ''))
