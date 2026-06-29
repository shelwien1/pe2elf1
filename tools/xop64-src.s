.intel_syntax noprefix
.text
vpcmov xmm0, xmm1, xmm2, xmm3
vpcmov ymm0, ymm1, ymm2, ymm3
vpperm xmm0, xmm1, xmm2, xmm3
vpmacsdd xmm0, xmm1, xmm2, xmm3
vprotb xmm0, xmm1, 0x3
vprotw xmm0, xmm1, 0x5
vprotd xmm0, xmm1, 0x7
vprotq xmm0, xmm1, 0x1
vprotb xmm0, xmm1, xmm2
vprotd xmm0, xmm1, xmm2
vpshlb xmm0, xmm1, xmm2
vpshlw xmm0, xmm1, xmm2
vpshld xmm0, xmm1, xmm2
vpshlq xmm0, xmm1, xmm2
vpshab xmm0, xmm1, xmm2
vpshad xmm0, xmm1, xmm2
vphaddbw xmm0, xmm1
vphaddwd xmm0, xmm1
vphadddq xmm0, xmm1
vphaddubw xmm0, xmm1
vphsubbw xmm0, xmm1
