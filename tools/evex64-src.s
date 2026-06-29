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
