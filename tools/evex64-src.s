.intel_syntax noprefix
.text
vaddps zmm0, zmm1, zmm2
vaddps ymm3, ymm4, ymm5
vaddps xmm6, xmm7, xmm8
vmulps zmm0, zmm1, zmm2
vmulps ymm3, ymm4, ymm5
vmulps xmm6, xmm7, xmm8
vsubps zmm0, zmm1, zmm2
vsubps ymm3, ymm4, ymm5
vsubps xmm6, xmm7, xmm8
vdivps zmm0, zmm1, zmm2
vdivps ymm3, ymm4, ymm5
vdivps xmm6, xmm7, xmm8
vminps zmm0, zmm1, zmm2
vminps ymm3, ymm4, ymm5
vminps xmm6, xmm7, xmm8
vmaxps zmm0, zmm1, zmm2
vmaxps ymm3, ymm4, ymm5
vmaxps xmm6, xmm7, xmm8
vandps zmm0, zmm1, zmm2
vandps ymm3, ymm4, ymm5
vandps xmm6, xmm7, xmm8
vorps zmm0, zmm1, zmm2
vorps ymm3, ymm4, ymm5
vorps xmm6, xmm7, xmm8
vxorps zmm0, zmm1, zmm2
vxorps ymm3, ymm4, ymm5
vxorps xmm6, xmm7, xmm8
vaddpd zmm0, zmm1, zmm2
vaddpd zmm10{k1}, zmm11, zmm12
vaddpd zmm0{k2}{z}, zmm1, zmm2
vmulpd zmm0, zmm1, zmm2
vmulpd zmm10{k1}, zmm11, zmm12
vmulpd zmm0{k2}{z}, zmm1, zmm2
vsubpd zmm0, zmm1, zmm2
vsubpd zmm10{k1}, zmm11, zmm12
vsubpd zmm0{k2}{z}, zmm1, zmm2
vdivpd zmm0, zmm1, zmm2
vdivpd zmm10{k1}, zmm11, zmm12
vdivpd zmm0{k2}{z}, zmm1, zmm2
vminpd zmm0, zmm1, zmm2
vminpd zmm10{k1}, zmm11, zmm12
vminpd zmm0{k2}{z}, zmm1, zmm2
vmaxpd zmm0, zmm1, zmm2
vmaxpd zmm10{k1}, zmm11, zmm12
vmaxpd zmm0{k2}{z}, zmm1, zmm2
vandpd zmm0, zmm1, zmm2
vandpd zmm10{k1}, zmm11, zmm12
vandpd zmm0{k2}{z}, zmm1, zmm2
vorpd zmm0, zmm1, zmm2
vorpd zmm10{k1}, zmm11, zmm12
vorpd zmm0{k2}{z}, zmm1, zmm2
vxorpd zmm0, zmm1, zmm2
vxorpd zmm10{k1}, zmm11, zmm12
vxorpd zmm0{k2}{z}, zmm1, zmm2
vaddps zmm31, zmm30, zmm29
vmulpd zmm16, zmm17, zmm18
vsubps zmm24{k3}, zmm8, zmm23
vaddps zmm0, zmm1, zmm2, {rn-sae}
vmulpd zmm5{k1}, zmm6, zmm7, {rn-sae}
vaddps zmm0, zmm1, zmm2, {rd-sae}
vmulpd zmm5{k1}, zmm6, zmm7, {rd-sae}
vaddps zmm0, zmm1, zmm2, {ru-sae}
vmulpd zmm5{k1}, zmm6, zmm7, {ru-sae}
vaddps zmm0, zmm1, zmm2, {rz-sae}
vmulpd zmm5{k1}, zmm6, zmm7, {rz-sae}
vaddss xmm0, xmm1, xmm2
vaddss xmm0{k1}, xmm1, xmm2
vaddss xmm0, xmm1, xmm2, {rn-sae}
vmulss xmm0, xmm1, xmm2
vmulss xmm0{k1}, xmm1, xmm2
vmulss xmm0, xmm1, xmm2, {rn-sae}
vsubss xmm0, xmm1, xmm2
vsubss xmm0{k1}, xmm1, xmm2
vsubss xmm0, xmm1, xmm2, {rn-sae}
vdivss xmm0, xmm1, xmm2
vdivss xmm0{k1}, xmm1, xmm2
vdivss xmm0, xmm1, xmm2, {rn-sae}
vaddsd xmm0, xmm1, xmm2
vaddsd xmm10, xmm11, xmm12, {rz-sae}
vmulsd xmm0, xmm1, xmm2
vmulsd xmm10, xmm11, xmm12, {rz-sae}
vsubsd xmm0, xmm1, xmm2
vsubsd xmm10, xmm11, xmm12, {rz-sae}
vdivsd xmm0, xmm1, xmm2
vdivsd xmm10, xmm11, xmm12, {rz-sae}
vaddps zmm0, zmm1, [rax]
vaddps zmm0, zmm1, [r8+rcx*4+0x40]
vaddps zmm0, zmm1, [rdx]{1to16}
vaddpd zmm0, zmm1, [rcx]{1to8}
vsubps ymm0, ymm1, [rax]{1to8}
vmulpd xmm0, xmm1, [rdx]{1to2}
vfmadd213ps zmm0, zmm1, [rax]{1to16}
vmovaps zmm0, zmm1
vmovaps zmm31{k1}, zmm16
vmovaps [rdi], zmm0
vmovaps zmm0, [rsi]
vmovapd zmm5{k2}{z}, zmm6
vmovups zmm0, [rax]
vmovupd [rdi]{k1}, zmm8
vmovdqa32 zmm0, zmm1
vmovdqa64 zmm0{k1}, [rax]
vmovdqu32 [rdi], zmm0
vmovdqu64 zmm8, zmm9
vmovdqu8 zmm0, zmm1
vmovdqu16 zmm5{k1}{z}, zmm6
vpaddd zmm0, zmm1, zmm2
vpaddq zmm0{k1}, zmm1, zmm2
vpaddb zmm0, zmm1, zmm2
vpaddw ymm0, ymm1, ymm2
vpsubd zmm0, zmm1, [rax]{1to16}
vpsubq zmm8, zmm9, zmm10
vpandd zmm0, zmm1, zmm2
vpandq zmm0, zmm1, [rax]{1to8}
vpandnd zmm0, zmm1, zmm2
vpord zmm0, zmm1, zmm2
vpxorq zmm0{k1}{z}, zmm1, zmm2
vpmulld zmm0, zmm1, zmm2
vpmullw zmm0, zmm1, zmm2
vfmadd213ps zmm0, zmm1, zmm2
vfmadd231pd zmm0{k1}, zmm1, zmm2
vfmadd132ps zmm0, zmm1, zmm2, {rn-sae}
vfmsub213pd zmm0, zmm1, zmm2
vfnmadd213ps zmm0, zmm1, zmm2
# ---- extended-register / memory edge cases ----
vaddps zmm0, zmm1, [r8]
vaddps zmm0, zmm1, [r15+r14*8+0x40]
vaddpd zmm0, zmm16, zmm2
vaddps zmm0, zmm31, zmm1
vmulps zmm0{k7}, zmm17, zmm18
vmovaps [r9], zmm0
vmovaps zmm0, [r12+0x100]
vmovdqu64 [r8+r9*1], zmm31
vpaddd zmm0, zmm1, [r10]{1to16}
vaddps zmm0{k1}{z}, zmm1, [rax]{1to16}
vfmadd231pd zmm16{k2}, zmm17, [r8]{1to8}
vaddss xmm16, xmm17, xmm18
vmovaps ymm0, ymm1
vmovaps xmm0, xmm1
vaddps zmm0, zmm1, zmm2, {ru-sae}
# ---- AVX-512 mask-register (k) ops ----
kmovw k1, k2
kmovw k1, eax
kmovw eax, k1
kmovw k1, [rax]
kmovw [rax], k1
kmovb k1, k2
kmovd k1, eax
kmovq k1, rax
kmovd eax, k1
kmovq rax, k1
kandw k1, k2, k3
kandb k1, k2, k3
korw k1, k2, k3
kxorw k1, k2, k3
kxnorw k1, k2, k3
kandnw k1, k2, k3
knotw k1, k2
kortestw k1, k2
ktestw k1, k2
kshiftlw k1, k2, 0x3
kshiftrw k1, k2, 0x3
kunpckbw k1, k2, k3
vpcmpeqd k1, zmm1, zmm2
vpcmpgtd k1{k2}, zmm1, zmm2
vcmpps k1, zmm1, zmm2, 0x0
vpcmpd k1, zmm1, zmm2, 0x5
vpcmpud k2, zmm1, zmm2, 0x2
vptestmd k1, zmm1, zmm2
# ---- AVX-512 round 2: converts / broadcasts / min-max / shifts / permutes ----
vcvtdq2ps zmm0, zmm1
vcvtdq2ps zmm2{k1}{z}, zmm3
vcvtdq2ps zmm0, zmm1, {rn-sae}
vcvtdq2ps zmm5, zmm6, {rz-sae}
vcvtdq2ps zmm0, [rax]{1to16}
vcvtdq2ps zmm0{k2}, [r8]{1to16}
vcvtps2dq zmm0, zmm1
vcvtps2dq zmm0, zmm1, {ru-sae}
vcvtps2dq zmm0, [rdx]
vcvttps2dq zmm0, zmm1
vcvttps2dq zmm0, zmm1, {sae}
vcvttps2dq zmm10, [rax]{1to16}
vcvtdq2ps zmm17, zmm28
vcvtps2dq zmm31{k7}, zmm16
vcvtss2sd xmm0{k1}, xmm1, xmm2
vcvtss2sd xmm0, xmm1, xmm2, {sae}
vcvtss2sd xmm16{k3}, xmm17, xmm18
vcvtsd2ss xmm0{k1}, xmm1, xmm2
vcvtsd2ss xmm0, xmm1, xmm2, {rn-sae}
vcvtsi2sd xmm0, xmm1, rax, {rn-sae}
vcvtsi2ss xmm0, xmm1, rax, {rz-sae}
vcvtsi2sd xmm0, xmm1, r8, {rn-sae}
vcvtsd2si rax, xmm1, {rn-sae}
vcvtsd2si r9, xmm1, {rd-sae}
vcvtss2si rax, xmm1, {ru-sae}
vcvttsd2si rax, xmm1, {sae}
vcvttss2si eax, xmm1, {sae}
vbroadcastss zmm0, xmm1
vbroadcastss zmm0{k1}, [rax]
vbroadcastsd zmm0, xmm1
vbroadcastsd zmm0, [rdx]
vpbroadcastd zmm0, xmm1
vpbroadcastd zmm0, eax
vpbroadcastd zmm0{k1}{z}, r8d
vpbroadcastq zmm0, rax
vpbroadcastq zmm0, xmm1
vpbroadcastq zmm5, r15
vpmaxsd zmm0, zmm1, zmm2
vpmaxsq zmm0, zmm1, zmm2
vpminsd zmm0, zmm1, [rax]{1to16}
vpmaxud zmm0, zmm1, zmm2
vpminud zmm0{k1}, zmm1, zmm2
vpmaxsb zmm0, zmm1, zmm2
vpminsw zmm0, zmm1, zmm2
vpmaxuw zmm0, zmm1, zmm2
vpminub zmm0, zmm1, zmm2
vpmaxsq zmm16, zmm17, zmm18
vpabsd zmm0, zmm1
vpabsq zmm0, [rax]{1to8}
vpabsb zmm0, zmm1
vpabsw zmm0, zmm1
vpslld zmm0, zmm1, 5
vpsrld zmm0{k1}, zmm1, 7
vpsrad zmm0, zmm1, 1
vpsllq zmm0, zmm1, 5
vpsraq zmm0, zmm1, 9
vpsrlq zmm0, zmm1, 3
vpsllw zmm0, zmm1, 2
vpsraw zmm0, zmm1, 4
vpslld zmm0, [rax]{1to16}, 5
vpslld zmm17, zmm28, 11
vpsllvd zmm0, zmm1, zmm2
vpsllvq zmm0, zmm1, zmm2
vpsrlvd zmm0, zmm1, zmm2
vpsrlvq zmm0, zmm1, zmm2
vpsravd zmm0, zmm1, [rax]{1to16}
vpsravq zmm0, zmm1, zmm2
vpermd zmm0, zmm1, zmm2
vpermps zmm0, zmm1, zmm2
vpermq zmm0, zmm1, 0x1b
vpermpd zmm0, zmm1, 0x1b
vpermq zmm0{k1}, [rax]{1to8}, 0x4e
# ---- AVX-512 round 3: ternlog / sqrt / mul / rndscale / scalef / rcp / getexp / blend / testnm ----
vpternlogd zmm0, zmm1, zmm2, 0xca
vpternlogq zmm0{k1}, zmm1, zmm2, 0xfe
vpternlogd zmm0, zmm1, [rax]{1to16}, 0x55
vsqrtps zmm0, zmm1
vsqrtps zmm0, zmm1, {rn-sae}
vsqrtpd zmm0{k1}{z}, zmm1
vsqrtps zmm0, [rax]{1to16}
vsqrtss xmm0{k1}, xmm1, xmm2
vsqrtss xmm0, xmm1, xmm2, {rn-sae}
vsqrtsd xmm0, xmm1, xmm2, {rz-sae}
vpmuldq zmm0, zmm1, zmm2
vpmuludq zmm0, zmm1, [rax]{1to8}
vpmullq zmm0, zmm1, zmm2
vrndscaleps zmm0, zmm1, 0x1
vrndscalepd zmm0{k1}, zmm1, 0x2
vrndscaleps zmm0, zmm1{sae}, 0x3
vrndscaless xmm0, xmm1, xmm2, 0x1
vrndscalesd xmm0, xmm1, xmm2, 0x2
vscalefps zmm0, zmm1, zmm2
vscalefpd zmm0, zmm1, zmm2, {rn-sae}
vscalefss xmm0, xmm1, xmm2
vrcp14ps zmm0, zmm1
vrcp14pd zmm0{k1}, zmm1
vrsqrt14ps zmm0, zmm1
vrcp14ss xmm0, xmm1, xmm2
vrsqrt14sd xmm0, xmm1, xmm2
vgetexpps zmm0, zmm1
vgetexppd zmm0, zmm1{sae}
vgetmantps zmm0, zmm1, 0x1
vgetmantpd zmm0, zmm1{sae}, 0x2
vpblendmd zmm0, zmm1, zmm2
vpblendmq zmm0{k1}, zmm1, zmm2
vblendmps zmm0, zmm1, zmm2
vblendmpd zmm0, zmm1, [rax]{1to8}
vptestnmd k1, zmm1, zmm2
vptestnmq k2{k1}, zmm1, zmm2
# ---- VEX vmovq xmm forms (load 7E / store D6, incl. extended-reg dual-encoding) ----
vmovq xmm0, xmm1
vmovq xmm0, [rax]
vmovq [rax], xmm0
vmovq xmm0, xmm9
vmovq xmm9, xmm0
vmovq xmm8, [r9]
vmovq [r9], xmm8
# ---- AVX-512 round 4: width-changing converts (operands differ in width) ----
vcvtps2pd zmm0, ymm1
vcvtps2pd zmm0{k1}{z}, ymm1
vcvtps2pd zmm0, ymm1{sae}
vcvtps2pd zmm0, [rax]{1to8}
vcvtps2pd zmm17, ymm28
vcvtpd2ps ymm0, zmm1
vcvtpd2ps ymm0, zmm1, {rn-sae}
vcvtpd2ps ymm0, [rax]{1to8}
vcvtdq2pd zmm0, ymm1
vcvtdq2pd zmm0, [rax]{1to8}
vcvtpd2dq ymm0, zmm1
vcvtpd2dq ymm0, zmm1, {rn-sae}
vcvtpd2dq ymm0, [rax]{1to8}
vcvttpd2dq ymm0, zmm1
vcvttpd2dq ymm0, zmm1{sae}
vcvttpd2dq ymm0, [rax]{1to8}
vcvtudq2pd zmm0, ymm1
# ---- AVX-512 round 5: integer width converts (vpmovzx/sx widen, vpmov narrow) ----
vpmovzxbw zmm0, ymm1
vpmovzxbd zmm0, xmm1
vpmovzxbq zmm0, xmm1
vpmovzxwd zmm0, ymm1
vpmovzxwq zmm0, xmm1
vpmovzxdq zmm0, ymm1
vpmovsxbw zmm0, ymm1
vpmovsxbd zmm0, xmm1
vpmovsxwd zmm0, ymm1
vpmovsxdq zmm0, ymm1
vpmovzxbd zmm0{k1}, xmm1
vpmovzxbd zmm0, [rax]
vpmovzxwd zmm17, ymm28
vpmovwb ymm0, zmm1
vpmovdb xmm0, zmm1
vpmovdb [rax], zmm1
vpmovqb xmm0, zmm1
vpmovqw xmm0, zmm1
vpmovqd ymm0, zmm1
vpmovdw ymm0, zmm1
vpmovsdb xmm0, zmm1
vpmovusdb xmm0, zmm1
vpmovsdw ymm0, zmm1
vpmovdb xmm0{k1}, zmm1
