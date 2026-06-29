evex vaddps zmm0,zmm1,zmm2
vaddps ymm3,ymm4,ymm5
vaddps xmm6,xmm7,xmm8
evex vmulps zmm0,zmm1,zmm2
vmulps ymm3,ymm4,ymm5
vmulps xmm6,xmm7,xmm8
evex vsubps zmm0,zmm1,zmm2
vsubps ymm3,ymm4,ymm5
vsubps xmm6,xmm7,xmm8
evex vdivps zmm0,zmm1,zmm2
vdivps ymm3,ymm4,ymm5
vdivps xmm6,xmm7,xmm8
evex vminps zmm0,zmm1,zmm2
vminps ymm3,ymm4,ymm5
vminps xmm6,xmm7,xmm8
evex vmaxps zmm0,zmm1,zmm2
vmaxps ymm3,ymm4,ymm5
vmaxps xmm6,xmm7,xmm8
evex vandps zmm0,zmm1,zmm2
vandps ymm3,ymm4,ymm5
vandps xmm6,xmm7,xmm8
evex vorps zmm0,zmm1,zmm2
vorps ymm3,ymm4,ymm5
vorps xmm6,xmm7,xmm8
evex vxorps zmm0,zmm1,zmm2
vxorps ymm3,ymm4,ymm5
vxorps xmm6,xmm7,xmm8
evex vaddpd zmm0,zmm1,zmm2
evex vaddpd zmm10 {k1},zmm11,zmm12
evex vaddpd zmm0 {k2} {z},zmm1,zmm2
evex vmulpd zmm0,zmm1,zmm2
evex vmulpd zmm10 {k1},zmm11,zmm12
evex vmulpd zmm0 {k2} {z},zmm1,zmm2
evex vsubpd zmm0,zmm1,zmm2
evex vsubpd zmm10 {k1},zmm11,zmm12
evex vsubpd zmm0 {k2} {z},zmm1,zmm2
evex vdivpd zmm0,zmm1,zmm2
evex vdivpd zmm10 {k1},zmm11,zmm12
evex vdivpd zmm0 {k2} {z},zmm1,zmm2
evex vminpd zmm0,zmm1,zmm2
evex vminpd zmm10 {k1},zmm11,zmm12
evex vminpd zmm0 {k2} {z},zmm1,zmm2
evex vmaxpd zmm0,zmm1,zmm2
evex vmaxpd zmm10 {k1},zmm11,zmm12
evex vmaxpd zmm0 {k2} {z},zmm1,zmm2
evex vandpd zmm0,zmm1,zmm2
evex vandpd zmm10 {k1},zmm11,zmm12
evex vandpd zmm0 {k2} {z},zmm1,zmm2
evex vorpd zmm0,zmm1,zmm2
evex vorpd zmm10 {k1},zmm11,zmm12
evex vorpd zmm0 {k2} {z},zmm1,zmm2
evex vxorpd zmm0,zmm1,zmm2
evex vxorpd zmm10 {k1},zmm11,zmm12
evex vxorpd zmm0 {k2} {z},zmm1,zmm2
evex vaddps zmm31,zmm30,zmm29
evex vmulpd zmm16,zmm17,zmm18
evex vsubps zmm24 {k3},zmm8,zmm23
evex vaddps zmm0,zmm1,zmm2 {rn-sae}
evex vmulpd zmm5 {k1},zmm6,zmm7 {rn-sae}
evex vaddps zmm0,zmm1,zmm2 {rd-sae}
evex vmulpd zmm5 {k1},zmm6,zmm7 {rd-sae}
evex vaddps zmm0,zmm1,zmm2 {ru-sae}
evex vmulpd zmm5 {k1},zmm6,zmm7 {ru-sae}
evex vaddps zmm0,zmm1,zmm2 {rz-sae}
evex vmulpd zmm5 {k1},zmm6,zmm7 {rz-sae}
vaddss xmm0,xmm1,xmm2
evex vaddss xmm0 {k1},xmm1,xmm2
evex vaddss zmm0,zmm1,zmm2 {rn-sae}
vmulss xmm0,xmm1,xmm2
evex vmulss xmm0 {k1},xmm1,xmm2
evex vmulss zmm0,zmm1,zmm2 {rn-sae}
vsubss xmm0,xmm1,xmm2
evex vsubss xmm0 {k1},xmm1,xmm2
evex vsubss zmm0,zmm1,zmm2 {rn-sae}
vdivss xmm0,xmm1,xmm2
evex vdivss xmm0 {k1},xmm1,xmm2
evex vdivss zmm0,zmm1,zmm2 {rn-sae}
vaddsd xmm0,xmm1,xmm2
evex vaddsd zmm10,zmm11,zmm12 {rz-sae}
vmulsd xmm0,xmm1,xmm2
evex vmulsd zmm10,zmm11,zmm12 {rz-sae}
vsubsd xmm0,xmm1,xmm2
evex vsubsd zmm10,zmm11,zmm12 {rz-sae}
vdivsd xmm0,xmm1,xmm2
evex vdivsd zmm10,zmm11,zmm12 {rz-sae}
evex vaddps zmm0,zmm1,[rax]
evex vaddps zmm0,zmm1,[r8+rcx*4+0x1]
evex vaddps zmm0,zmm1,[rdx] {1to16}
evex vaddpd zmm0,zmm1,[rcx] {1to8}
evex vsubps ymm0,ymm1,[rax] {1to8}
evex vmulpd xmm0,xmm1,[rdx] {1to2}
evex vfmadd213ps zmm0,zmm1,[rax] {1to16}
evex vmovaps zmm0,zmm1
evex vmovaps zmm31 {k1},zmm16
evex vmovaps [rdi],zmm0
evex vmovaps zmm0,[rsi]
evex vmovapd zmm5 {k2} {z},zmm6
evex vmovups zmm0,[rax]
evex vmovupd [rdi] {k1},zmm8
evex vmovdqa32 zmm0,zmm1
evex vmovdqa64 zmm0 {k1},[rax]
evex vmovdqu32 [rdi],zmm0
evex vmovdqu64 zmm8,zmm9
evex vmovdqu8 zmm0,zmm1
evex vmovdqu16 zmm5 {k1} {z},zmm6
evex vpaddd zmm0,zmm1,zmm2
evex vpaddq zmm0 {k1},zmm1,zmm2
evex vpaddb zmm0,zmm1,zmm2
vpaddw ymm0,ymm1,ymm2
evex vpsubd zmm0,zmm1,[rax] {1to16}
evex vpsubq zmm8,zmm9,zmm10
evex vpandd zmm0,zmm1,zmm2
evex vpandq zmm0,zmm1,[rax] {1to8}
evex vpandnd zmm0,zmm1,zmm2
evex vpord zmm0,zmm1,zmm2
evex vpxorq zmm0 {k1} {z},zmm1,zmm2
evex vpmulld zmm0,zmm1,zmm2
evex vpmullw zmm0,zmm1,zmm2
evex vfmadd213ps zmm0,zmm1,zmm2
evex vfmadd231pd zmm0 {k1},zmm1,zmm2
evex vfmadd132ps zmm0,zmm1,zmm2 {rn-sae}
evex vfmsub213pd zmm0,zmm1,zmm2
evex vfnmadd213ps zmm0,zmm1,zmm2
evex vaddps zmm0,zmm1,[r8]
evex vaddps zmm0,zmm1,[r15+r14*8+0x1]
evex vaddpd zmm0,zmm16,zmm2
evex vaddps zmm0,zmm31,zmm1
evex vmulps zmm0 {k7},zmm17,zmm18
evex vmovaps [r9],zmm0
evex vmovaps zmm0,ss:[r12+0x4]
evex vmovdqu64 [r8+r9*1],zmm31
evex vpaddd zmm0,zmm1,[r10] {1to16}
evex vaddps zmm0 {k1} {z},zmm1,[rax] {1to16}
evex vfmadd231pd zmm16 {k2},zmm17,[r8] {1to8}
evex vaddss xmm16,xmm17,xmm18
vmovaps ymm0,ymm1
vmovaps xmm0,xmm1
evex vaddps zmm0,zmm1,zmm2 {ru-sae}
