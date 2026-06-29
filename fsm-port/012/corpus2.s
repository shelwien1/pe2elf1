/* corpus2.s -- a broad 32-bit instruction corpus, assembled with GAS to get
 * real, canonical encodings (vs corpus.bin's hand-built non-canonical ones).
 * Emphasis on opcodes BEYOND corpus.bin, to drive porting from corpus1.p.
 *
 *   as --32 corpus2.s -o corpus2.o
 *   objcopy -O binary -j .text corpus2.o corpus2.bin
 */
	.text
	.intel_syntax noprefix

/* ---- ALU: every op, register + memory + immediate forms ---- */
	add    eax, ebx
	add    ecx, DWORD PTR [edx]
	add    DWORD PTR [esi], edi
	or     eax, 0x12345678
	adc    bl, cl
	sbb    edx, DWORD PTR [ebx+4]
	and    eax, 0x10
	sub    esp, 0x20
	xor    ah, bh
	cmp    DWORD PTR [edi+esi*4+8], 0x99

/* ---- shifts / rotates: imm, 1, cl ---- */
	shl    eax, 5
	shr    ebx, 1
	sar    DWORD PTR [ecx], cl
	rol    dl, 3
	ror    esi, 1
	rcl    edi, cl
	rcr    BYTE PTR [eax], 7

/* ---- mul/div/neg/not, imul forms ---- */
	mul    ecx
	imul   ebx
	div    esi
	idiv   edi
	neg    eax
	not    DWORD PTR [edx]
	imul   eax, ebx
	imul   ecx, edx, 0x10
	imul   esi, DWORD PTR [edi], 0x1234

/* ---- inc/dec/test ---- */
	inc    eax
	dec    ebx
	inc    DWORD PTR [ecx]
	test   eax, ebx
	test   ecx, 0x40
	test   BYTE PTR [edx], 0x8

/* ---- mov / movzx / movsx / lea ---- */
	mov    eax, ebx
	mov    DWORD PTR [eax], ecx
	mov    edx, 0x12345678
	movzx  eax, bl
	movzx  ecx, WORD PTR [edx]
	movsx  esi, bl
	movsx  edi, WORD PTR [eax]
	lea    eax, [ebx+ecx*4+0x10]

/* ---- bit ops ---- */
	bt     eax, ebx
	bts    DWORD PTR [ecx], edx
	btr    esi, 5
	btc    edi, 31
	bsf    eax, ebx
	bsr    ecx, DWORD PTR [edx]
	shld   eax, ebx, 4
	shrd   ecx, edx, cl

/* ---- xchg / xadd / cmpxchg / bswap (lock-able) ---- */
	xchg   eax, ecx
	xchg   DWORD PTR [eax], ebx
	xadd   DWORD PTR [ecx], edx
	cmpxchg DWORD PTR [esi], edi
	cmpxchg8b QWORD PTR [eax]
	bswap  edx
	lock xadd DWORD PTR [eax], ebx

/* ---- push / pop / stack ---- */
	push   eax
	push   0x12345678
	push   0x7f
	pop    ebx
	push   DWORD PTR [eax]
	pop    DWORD PTR [ecx]
	pusha
	popa
	pushf
	popf
	enter  0x10, 0
	leave

/* ---- control flow ---- */
	jmp    1f
	call   2f
1:	jz     1b
	jnz    2f
	jg     1b
	jle    2f
2:	loop   1b
	jecxz  1b
	ret
	ret    0x10
	retf
	int    0x80
	int3
	into
	iret

/* ---- setcc / cmovcc ---- */
	sete   al
	setne  bl
	setg   BYTE PTR [eax]
	cmove  eax, ebx
	cmovne ecx, DWORD PTR [edx]
	cmovl  esi, edi

/* ---- string ops + rep ---- */
	movsb
	movsd
	stosb
	stosd
	lodsd
	scasb
	cmpsd
	rep    movsd
	repe   cmpsb
	repne  scasb

/* ---- flags / misc ---- */
	clc
	stc
	cmc
	cld
	std
	cli
	sti
	sahf
	lahf
	cdq
	cwde
	nop
	hlt
	xlat

/* ---- far-pointer loads, bound, arpl ---- */
	les    eax, FWORD PTR [ebx]
	lds    ecx, FWORD PTR [edx]
	lss    esi, FWORD PTR [eax]
	lfs    edi, FWORD PTR [ecx]
	lgs    eax, FWORD PTR [edx]
	bound  eax, QWORD PTR [ebx]
	arpl   bx, ax

/* ---- I/O ---- */
	in     al, 0x60
	in     eax, dx
	out    0x60, al
	out    dx, eax

/* ---- system / privileged (0F 00, 0F 01, lar/lsl, clts, invd...) ---- */
	cpuid
	rdtsc
	rdmsr
	wrmsr
	ud2
	lar    eax, ebx
	lsl    ecx, edx
	clts
	invd
	wbinvd
	sgdt   [eax]
	sidt   [ecx]
	lgdt   [edx]
	lidt   [esi]
	smsw   eax
	lmsw   ax
	invlpg [eax]
	sldt   ax
	str    bx
	lldt   cx
	ltr    dx
	verr   ax
	verw   bx

/* ---- x87: arithmetic (mem + st(i)), loads/stores, compares, specials ---- */
	fld    DWORD PTR [eax]
	fld    QWORD PTR [ebx]
	fld    TBYTE PTR [ecx]
	fld    st(2)
	fst    DWORD PTR [eax]
	fstp   QWORD PTR [ebx]
	fadd   DWORD PTR [eax]
	fadd   st, st(1)
	fadd   st(2), st
	faddp  st(1), st
	fsub   st, st(3)
	fsubr  st, st(4)
	fmul   QWORD PTR [eax]
	fdiv   st, st(5)
	fdivr  st(6), st
	fild   DWORD PTR [eax]
	fild   QWORD PTR [ecx]
	fistp  DWORD PTR [edx]
	ficom  DWORD PTR [eax]
	fcom   st(1)
	fcomp  st(2)
	fcompp
	fucom  st(3)
	fucomp st(4)
	fucompp
	fcomi  st, st(1)
	fcomip st, st(2)
	fucomi st, st(3)
	fchs
	fabs
	ftst
	fxam
	fld1
	fldz
	fldpi
	fldl2e
	fldln2
	f2xm1
	fyl2x
	fptan
	fpatan
	fsqrt
	fsin
	fcos
	fsincos
	fscale
	fprem
	frndint
	fxtract
	fnstsw ax
	fnstcw WORD PTR [eax]
	fldcw  WORD PTR [ebx]
	fninit
	fnclex
	fxch   st(1)
	ffree  st(2)
	fincstp
	fdecstp
	fcmovb  st, st(1)
	fcmove  st, st(2)
	fcmovbe st, st(3)
	fcmovnb st, st(4)

/* ---- MMX: moves, packed arithmetic, logical, compare, pack/unpack, shift ---- */
	movd   mm0, eax
	movd   eax, mm1
	movq   mm0, mm1
	movq   mm2, QWORD PTR [eax]
	paddb  mm0, mm1
	paddw  mm0, mm1
	paddd  mm0, mm1
	paddsb mm0, mm1
	paddusb mm0, mm1
	psubb  mm0, mm1
	psubw  mm0, mm1
	psubd  mm0, mm1
	pmullw mm0, mm1
	pmulhw mm0, mm1
	pmaddwd mm0, mm1
	pand   mm0, mm1
	pandn  mm0, mm1
	por    mm0, mm1
	pxor   mm0, mm1
	pcmpeqb mm0, mm1
	pcmpgtw mm0, mm1
	packsswb mm0, mm1
	packuswb mm0, mm1
	punpcklbw mm0, mm1
	punpckhwd mm0, mm1
	psllw  mm0, 4
	psrld  mm0, mm1
	psraw  mm0, mm1
	emms

/* ---- SSE / SSE2 ---- */
	movaps xmm0, xmm1
	movups xmm0, XMMWORD PTR [eax]
	movss  xmm0, DWORD PTR [eax]
	movsd  xmm0, QWORD PTR [eax]
	movd   xmm0, eax
	movq   xmm0, xmm1
	movdqa xmm0, xmm1
	movdqu xmm0, XMMWORD PTR [eax]
	addps  xmm0, xmm1
	addss  xmm0, xmm1
	addpd  xmm0, xmm1
	addsd  xmm0, xmm1
	subps  xmm0, xmm1
	mulps  xmm0, xmm1
	divps  xmm0, xmm1
	minps  xmm0, xmm1
	maxps  xmm0, xmm1
	sqrtps xmm0, xmm1
	andps  xmm0, xmm1
	andnps xmm0, xmm1
	orps   xmm0, xmm1
	xorps  xmm0, xmm1
	cmpps  xmm0, xmm1, 0
	shufps xmm0, xmm1, 0x1b
	unpcklps xmm0, xmm1
	unpckhps xmm0, xmm1
	cvtsi2ss xmm0, eax
	cvtss2si eax, xmm0
	cvttss2si eax, xmm0
	cvtpi2ps xmm0, mm1
	comiss xmm0, xmm1
	ucomisd xmm0, xmm1
	pshufd xmm0, xmm1, 0x4e
	pshuflw xmm0, xmm1, 0x1b
	paddb  xmm0, xmm1
	pxor   xmm0, xmm1
	pmuludq xmm0, xmm1
	pcmpeqd xmm0, xmm1
	pmovmskb eax, xmm1
	pslld  xmm0, 8
	movmskps eax, xmm1
	andpd  xmm0, xmm1

/* ---- prefetch / fences / clflush ---- */
	prefetchnta [eax]
	prefetcht0  [eax]
	lfence
	mfence
	sfence
	clflush [eax]

/* ---- redundant prefixes / segment overrides (canonical opcodes, GAS-emitted) ---- */
	mov    eax, fs:[0x30]
	mov    eax, gs:[ecx]
	es mov DWORD PTR [eax], ebx

/* ---- three-byte 0F 38: SSSE3 + SSE4.1/4.2 packed ops (reg + mem) ---- */
	pshufb    xmm0, xmm1
	pshufb    mm0, mm1
	phaddw    xmm0, xmm1
	phaddd    xmm2, XMMWORD PTR [eax]
	phaddsw   xmm0, xmm1
	pmaddubsw xmm0, xmm1
	phsubw    xmm0, xmm1
	phsubd    xmm0, xmm1
	psignb    xmm0, xmm1
	psignd    xmm0, xmm1
	pmulhrsw  xmm0, xmm1
	pabsb     xmm0, xmm1
	pabsd     xmm2, XMMWORD PTR [ecx]
	pmuldq    xmm0, xmm1
	pcmpeqq   xmm0, xmm1
	pcmpgtq   xmm0, xmm1
	packusdw  xmm0, xmm1
	pminsb    xmm0, xmm1
	pminud    xmm0, xmm1
	pmaxsb    xmm0, xmm1
	pmaxud    xmm0, xmm1
	pmulld    xmm0, xmm1
	phminposuw xmm0, xmm1
	ptest     xmm0, xmm1
	pmovsxbw  xmm0, xmm1
	pmovsxbd  xmm0, xmm1
	pmovsxdq  xmm0, xmm1
	pmovzxbw  xmm0, xmm1
	pmovzxwd  xmm2, QWORD PTR [eax]
	pmovzxdq  xmm0, xmm1
	movntdqa  xmm0, XMMWORD PTR [eax]
	blendvps  xmm0, xmm1
	aesenc    xmm0, xmm1
	aesdeclast xmm0, xmm1
	aesimc    xmm0, xmm1
	sha256msg1 xmm0, xmm1

/* ---- three-byte 0F 3A: SSE4.1/4.2 imm8 ops ---- */
	roundps   xmm0, xmm1, 0x3
	roundsd   xmm0, xmm1, 0x1
	blendps   xmm0, xmm1, 0x5
	blendpd   xmm0, xmm1, 0xa
	pblendw   xmm0, xmm1, 0x33
	palignr   xmm2, xmm3, 0x4
	palignr   mm0, mm1, 0x2
	insertps  xmm0, xmm1, 0x10
	dpps      xmm0, xmm1, 0xff
	dppd      xmm0, xmm1, 0x31
	mpsadbw   xmm0, xmm1, 0x0
	pclmulqdq xmm0, xmm1, 0x11
	pcmpestri xmm0, xmm1, 0x0c
	pcmpistri xmm0, xmm1, 0x1a
	aeskeygenassist xmm0, xmm1, 0x7

/* ---- pextr/pinsr (mixed greg<->xmm), two-byte and three-byte forms ---- */
	pextrb    eax, xmm0, 0x1
	pextrd    ecx, xmm1, 0x2
	extractps edx, xmm2, 0x3
	pextrw    eax, xmm0, 0x4
	pextrw    esi, xmm3, 0x5
	pinsrb    xmm0, eax, 0x1
	pinsrd    xmm1, ecx, 0x2
	pinsrw    xmm0, eax, 0x6
	pinsrw    xmm2, WORD PTR [eax], 0x7
	pextrb    BYTE PTR [eax], xmm0, 0x1

/* ---- SSE3 / packed-cvt / non-temporal / 0F AE mem group ---- */
	addsubps  xmm0, xmm1
	haddps    xmm0, xmm1
	hsubps    xmm0, xmm1
	movddup   xmm0, xmm1
	movshdup  xmm0, xmm1
	movsldup  xmm0, xmm1
	lddqu     xmm0, [eax]
	fisttp    DWORD PTR [eax]
	punpcklqdq xmm0, xmm1
	punpckhqdq xmm0, xmm1
	movhlps   xmm0, xmm1
	movlhps   xmm0, xmm1
	cvtdq2ps  xmm0, xmm1
	cvtps2dq  xmm0, xmm1
	cvttps2dq xmm0, xmm1
	cvtpd2ps  xmm0, xmm1
	cvtps2pd  xmm0, xmm1
	cvtsd2ss  xmm0, xmm1
	cvtss2sd  xmm0, xmm1
	cvtsi2sd  xmm0, eax
	cvtsd2si  eax, xmm0
	movnti    DWORD PTR [eax], ecx
	movntps   XMMWORD PTR [eax], xmm0
	movntdq   XMMWORD PTR [eax], xmm0
	movntq    QWORD PTR [eax], mm0
	maskmovdqu xmm0, xmm1
	ldmxcsr   DWORD PTR [eax]
	stmxcsr   DWORD PTR [eax]
	fxsave    [eax]
	fxrstor   [eax]

/* ---- system specials + popcnt + movbe/crc32 ---- */
	monitor
	mwait
	rdtscp
	sysenter
	sysexit
	rsm
	rdpmc
	clac
	stac
	popcnt eax, ecx
	lzcnt  eax, ecx
	tzcnt  eax, ecx
	movbe  eax, DWORD PTR [esi]
	movbe  DWORD PTR [edi], ecx
	crc32  eax, bl
	crc32  eax, ecx
	crc32  eax, DWORD PTR [esi]

/* ---- cr/dr moves, multibyte nop, prefetch, 0F 01 specials, ud1, D6, int1 ---- */
# 0F 01 reg-form specials (xgetbv/xsetbv/vmcall/...)
xgetbv
xsetbv
vmcall
vmlaunch
vmresume
vmxoff
# undefined-opcode markers
ud2
ud1 eax, ecx
# x87 misc that may be mnemonic-placeholdered
fnsave  [eax]
frstor  [eax]
fnstenv [eax]
fldenv  [eax]
fbld    [eax]
fbstp   [eax]
# moves between mm/xmm
movq2dq xmm0, mm1
movdq2q mm0, xmm1
movq    xmm0, xmm1
movq    QWORD PTR [eax], xmm0
# hint nops / prefetch
prefetchw [eax]
nop  DWORD PTR [eax]
nop  WORD PTR [eax]
# misc one-byte
int1
# bit/byte
movbe ax, WORD PTR [eax]
# string scan with addr override already covered; segment moves
mov  eax, cr0
mov  cr0, eax
mov  eax, dr0
mov  dr0, eax
# fxsave64-ish not in 32-bit; xsaveopt covered
# rotates through carry already covered
# bsf/bsr/popcnt covered; lzcnt/tzcnt covered

/* ---- rdrand/rdseed, getsec, x87 full sweep, maskmovq ---- */
# rdrand/rdseed (0F C7 /6 /7 reg)
rdrand eax
rdseed ecx
# syscall/sysret (AMD), getsec, encls/enclu
getsec
# x87 full sweep (arith both directions, all stack ops)
fadd  st(0), st(1)
fadd  st(3), st(0)
fmul  st(0), st(2)
fsub  st(0), st(4)
fsubr st(0), st(5)
fdiv  st(0), st(6)
fdivr st(0), st(7)
fsubrp st(1), st(0)
fdivrp st(2), st(0)
fcom  st(1)
fcomp st(2)
fucom st(3)
ffree st(4)
fxch  st(5)
fst   st(2)
fstp  st(3)
fld   st(4)
fcmovb st(0), st(1)
fcmovnu st(0), st(2)
ffreep st(1)
fnstsw ax
fnstcw WORD PTR [eax]
fldcw  WORD PTR [ebx]
fiadd  DWORD PTR [eax]
fimul  WORD PTR [eax]
ficomp DWORD PTR [eax]
fisub  DWORD PTR [eax]
fidivr WORD PTR [eax]
fadd   DWORD PTR [eax]
fadd   QWORD PTR [eax]
# masked move + movntdq dest
maskmovq mm0, mm1

/* ---- 16-bit addressing, cmovcc, 0F 38/3A mem forms, misc ---- */
# 16-bit addressing (67 prefix)
add  ax, WORD PTR [bx+si]
mov  cx, WORD PTR [bp+di+4]
lea  eax, [bx+0x10]
mov  al, BYTE PTR [si]
# all cmovcc
cmovo  eax, ecx
cmovno eax, ecx
cmovb  eax, ecx
cmovae eax, ecx
cmovs  eax, ecx
cmovp  eax, ecx
# 0F 38 remaining (invept-ish are vex; try pmuldq mem, ptest mem)
ptest    xmm0, XMMWORD PTR [eax]
pmuldq   xmm0, XMMWORD PTR [eax]
pshufb   xmm0, XMMWORD PTR [ecx]
# 0F 3A mem forms + imm
roundps  xmm0, XMMWORD PTR [eax], 0x2
palignr  xmm0, XMMWORD PTR [eax], 0x3
pblendw  xmm0, XMMWORD PTR [eax], 0x4
# rare but legal
xlat
bswap  ecx
rdtsc
cpuid
wbinvd
# 66 push/pop (16-bit)
push   ax
pop    bx
# loop family
loopne 1f
loope  1f
1: jecxz 1b
# btc/bts imm + mem
bts    DWORD PTR [eax], 5
btc    DWORD PTR [ecx], 7
# neg/not/mul/div mem
neg    DWORD PTR [eax]
mul    BYTE PTR [ecx]

/* ---- SSE move stores, syscall, adcx/adox, gf2p8, vmread/write, ud0, endbr ---- */
movups XMMWORD PTR [eax], xmm0
movaps XMMWORD PTR [ecx], xmm1
movlps QWORD PTR [eax], xmm0
movhps QWORD PTR [eax], xmm2
movdqa XMMWORD PTR [eax], xmm0
movdqu XMMWORD PTR [eax], xmm1
movq   QWORD PTR [eax], mm0
syscall
sysret
adcx   eax, ecx
adox   eax, ecx
gf2p8mulb xmm0, xmm1
gf2p8affineqb xmm0, xmm1, 0x5
invept eax, XMMWORD PTR [ecx]
movdiri DWORD PTR [eax], ecx
vmread eax, ecx
vmwrite eax, ecx
ud0 eax, ecx
endbr32
endbr64

/* ---- three-byte group (0F 38 D8 key-locker wide) via raw bytes ---- */
	.byte 0x0f, 0x38, 0xd8, 0x08          /* aesencwide128kl [eax] */
	.byte 0x0f, 0x38, 0xd8, 0x4c, 0x24, 0x10  /* /1 [esp+0x10] */

/* ---- xabort/xbegin, xrstors/xsavec/xsaves, vmptrld/vmptrst, ptwrite ---- */
xabort 0x5
xbegin 1f
1:
xrstors [eax]
xsavec  [eax]
xsaves  [eax]
vmptrld QWORD PTR [eax]
vmptrst QWORD PTR [eax]
ptwrite eax
/* rstorssp, prefetchit0/1, 0F 18 reserved hints, 0F 01 /5 via raw bytes */
	.byte 0xf3, 0x0f, 0x01, 0x28        /* rstorssp [eax] (0F 01 /5) */
	.byte 0x0f, 0x18, 0x30             /* prefetchit1 [eax] (0F 18 /6) */
	.byte 0x0f, 0x18, 0x38             /* prefetchit0 [eax] (0F 18 /7) */
	.byte 0x0f, 0x18, 0x20             /* nop [eax] (0F 18 /4) */
	.byte 0x0f, 0x1c, 0x00   /* cldemote [eax] (0F 1C /0) */

/* ---- 3DNow! (0F 0F suffix-opcode) via raw bytes ---- */
	.byte 0x0f, 0x0f, 0xc1, 0xbf          /* pavgusb mm0, mm1 */
	.byte 0x0f, 0x0f, 0xc1, 0x9e          /* pfadd   mm0, mm1 */
	.byte 0x0f, 0x0f, 0x00, 0xb4          /* pfmul   mm0, [eax] */
	.byte 0x0f, 0x0f, 0x44, 0x24, 0x08, 0x9a  /* pfsub mm0, [esp+8] */

/* ---- VEX (AVX/AVX2/FMA/BMI): C4/C5 prefix, all 3 maps, imm, ymm, W1 ---- */
vmovups xmm0, xmm1
vaddps  xmm0, xmm1, xmm2
vmovaps ymm0, ymm1
vxorps  xmm0, xmm0, [eax]
vmulps  ymm3, ymm4, [ebx+ecx*4+0x10]
vcmpps  xmm0, xmm1, xmm2, 3
vshufps xmm0, xmm1, xmm2, 0x1b
vpshufb xmm0, xmm1, xmm2
vpaddd  ymm0, ymm1, ymm2
vpalignr xmm0, xmm1, xmm2, 5
vzeroupper
vzeroall
vbroadcastss ymm0, [eax]
vpinsrw xmm0, xmm1, eax, 2
# 0F map, 2-byte and 3-byte VEX, xmm/ymm
vmovups xmm0, [eax]
vmovupd ymm1, [ebx+ecx*8+0x100]
vmovss  xmm0, xmm1, xmm2
vaddsd  xmm0, xmm1, [esi]
vsqrtps ymm7, [edi]
vandnps xmm3, xmm4, xmm5
vunpcklps ymm0, ymm1, ymm2
vmovmskps eax, ymm3
vcvtsi2sd xmm0, xmm1, ecx
vcvttss2si eax, xmm2
# imm forms
vcmpeqps  ymm0, ymm1, ymm2
vshufpd   ymm0, ymm1, ymm2, 5
vblendps  ymm0, ymm1, ymm2, 0xa
vinsertf128 ymm0, ymm1, xmm2, 1
vperm2f128  ymm0, ymm1, ymm2, 0x20
vpermilps   ymm0, ymm1, 0x1b
vextractf128 xmm0, ymm1, 1
vpblendw  xmm0, xmm1, xmm2, 0xc3
vroundss  xmm0, xmm1, xmm2, 2
# 0F38: FMA, gather-ish, broadcasts
vfmadd132ps ymm0, ymm1, ymm2
vfmadd213sd xmm0, xmm1, xmm2
vpbroadcastd ymm0, xmm1
vbroadcastsd ymm0, [eax]
vpmaskmovd  xmm0, xmm1, [eax]
vphaddw     xmm0, xmm1, xmm2
# BMI (VEX.LZ on GPR)
andn  eax, ebx, ecx
bextr eax, ebx, ecx
blsi  eax, ebx
blsmsk eax, ebx
mulx  eax, ebx, ecx
pdep  eax, ebx, ecx
shlx  eax, ebx, ecx
rorx  eax, ebx, 7
# W1 forms (VEX3)
vpermq ymm0, ymm1, 0x1b
vpsllvq xmm0, xmm1, xmm2

/* ---- EVEX (AVX-512): 62 prefix, zmm/ymm/xmm, {k}{z} masks, broadcast, imm ---- */
vaddps   zmm0, zmm1, zmm2
vmovups  zmm0, [eax]
vmovaps  ymm0, ymm1
vpaddd   xmm0, xmm1, xmm2
vaddpd   zmm0 {k1}, zmm1, zmm2
vaddps   zmm0 {k2}{z}, zmm1, zmm2
vmulps   zmm0, zmm1, [ebx+ecx*4+0x80]
vfmadd132ps zmm0, zmm1, zmm2
vpternlogd  zmm0, zmm1, zmm2, 0x55
vshufps  zmm0, zmm1, zmm2, 0x1b
vbroadcastss zmm0, xmm1
vcmpps   k1, zmm1, zmm2, 4
vmovdqa64 zmm0, [eax]
vpermd   ymm0, ymm1, ymm2
vrndscaleps zmm0, zmm1, 0x3
vaddps   zmm0, zmm1, dword ptr [eax]{1to16}

/* ---- XOP (AMD): 8F prefix, maps 8/9/10, is4 4th-operand, imm ---- */
vpmacssww xmm0, xmm1, xmm2, xmm3
vpcmov    xmm0, xmm1, xmm2, xmm3
vprotb    xmm0, xmm1, 5
vpshlw    xmm0, xmm1, xmm2
vphaddbw  xmm0, xmm1
vfrczps   ymm0, ymm1
bextr     eax, ebx, 0x10ff
