;=============================================================================
;  Model<0>::do_process  --  clang-18 -O3 -march=skylake (AVX2), MS x64 ABI
;
;  C source: model0.inc.  The rangecoder body is rc.inc resolved by
;  mk_kernel.sh into rc_vecD.inc, here with RC_VECOUT=1, RC_CARRYLESS=1,
;  RCNUM=16, RC_LOWSPLIT=1, RC_LOWBYTES=8, RC_SHIFT_SAT=0, and
;  RC_SCATTER=1 / RC_SCATTER_W=0 (staged store, scalar commit).
;
;  Everything the coder writes goes through ONE base: r13 = rcx+11000h =
;  tmpbase = rcio.rows().  r14 = this.  In the hot loop the base folds into
;  the addressing mode as the +11000h displacement off r14.
;
;-----------------------------------------------------------------------------
;  MAP  (offsets from the function start; search for the ==== banners)
;
;    +000h                  prologue: frame, xmm6-15 save, `and rsp,-64`,
;                           P_Init(stats), tmpbase into r13
;    +0EEh                  loop-invariant address table for the INLINED
;                           host_block -- 55 lea/mov pairs, once per call
;    +660h  loc_1400C0A20   carry fallback: recode_with_carry
;    +672h  loc_1400C0A32   `if( blksize<BLKSIZE ) break;`
;    +680h  loc_1400C0A40   BLOCK LOOP HEAD: chkinp / yield / getinplen /
;                           model_pass, then the -C dispatch on g_use_vec
;    +6DDh                  host_block: 16 x rc_Init, flag bit        [COLD]
;    +E30h  loc_1400C11F0   VECTOR per-block init; flag bit; and, only for a
;                           short last block, the 16-bit length header
;    +1179h loc_1400C1539   sweep preamble: lane state hoisted into ymm
;    +11F0h loc_1400C15B0   ****  THE HOT LOOP -- 16 coded bits per pass  ****
;    +1670h loc_1400C1A30   scalar length header                      [COLD]
;    +17A0h loc_1400C1B60   scalar main group loop                    [COLD]
;    +18D0h loc_1400C1C90   scalar tail loop                          [COLD]
;    +19F5h loc_1400C1DB5   vector tail: runtime alias checks, then an
;    +1A60h loc_1400C1E20     8-bits-at-a-time vectorised remainder
;    +1C70h loc_1400C2030   16 x rc_Quit for the vector path (minimal flush)
;    +1FC0h loc_1400C2380   lens[] / overflow / carry folds, then rc_Write
;    +2160h loc_1400C2520   16 x rc_Quit + folds + rc_Write for -C    [COLD]
;    +2340h loc_1400C2700   vector tail remainder, one bit at a time
;    +2454h loc_1400C2814   final yield + epilogue
;    +24C2h loc_1400C2882   rc_overflow (noreturn)
;
;  Only two blocks run per coded bit: Model<0>::model_pass (a separate
;  function -- it is NOINLINE on purpose) and loc_1400C15B0.  Everything else
;  in this listing is per block (65536 bytes) or per call, i.e. under 0.01
;  instructions per input byte.  Do not spend time on it.
;
;-----------------------------------------------------------------------------
;  WHAT THE HOT LOOP COSTS  (loc_1400C15B0: 145 instructions per 16 bits)
;
;      29 stores  = 16 payload (RC_FLUSHALL) + 13 spills
;      30 loads   = 2 pbit + 14 staged-array reloads + 14 spill reloads
;      ~17 p5 uops (10 vpextrd, 3 vextracti128, 2 vpmovzxwd, 2 vpmovsxwd)
;      ~158 fused uops
;
;  misc/avx2_profile.md measures the sweep at 38.06 cyc/group on a 3.235 GHz
;  Skylake, i.e. ~4.1 fused uops/cycle against a 4-wide rename.  So:
;
;      rename    ~100%    <-- the binding constraint
;      p4 store    76%    (29 stores / 38 cycles)
;      p0+p1+p5    75%
;      p5          45%
;      p23         39%
;
;  Nothing here stalls.  The loop-carried chains are ~25 cycles (lanes 0-7,
;  the half whose `range>>15` gets spilled) and ~20 (lanes 8-15), both well
;  under the 38 the loop actually takes.  THE ONLY LEVER IS A SHORTER
;  INSTRUCTION STREAM -- anything that adds uops to buy scheduling loses,
;  which is what every rejected experiment in avx2_profile.md section 8 did.
;
;-----------------------------------------------------------------------------
;  WHAT COULD BE IMPROVED  (best first; sizes in uops per 16 coded bits)
;
;  1. REGISTER PRESSURE IS THE WHOLE STORY -- AND THE FIX FOR IT LOSES.
;     13 spill stores + 14 spill reloads = 27 of ~158 uops, ~17% of the loop,
;     and they exist only because the live vector state does not fit: 6 lane
;     arrays (range, lowl, lowh, rpre, FFNum, tmpptr) x 2 ymm = 12, plus the
;     stcl/stad staging x 2 = 4, plus 3 broadcast constants = 19 ymm in a
;     16-register file.
;
;     The one structural way to shrink that: apply rc_Process's `rpre` to low
;     where it is PRODUCED, instead of leaving it pending for the next
;     ShiftLow to add.  Nothing reads low in between, so it is the same
;     arithmetic in the same order -- the carry detection and the FFNum mask
;     move with it -- and rpre stops being loop-carried.  Encoder-only, and
;     free of the decoder: f_DEC is a TEMPLATE parameter, so `if( f_DEC==0 )`
;     compiles the decoder's `code -= rpre` path unchanged.
;
;     That is RC_FOLD_RPRE in rc_config.inc now, and it does exactly what it
;     was meant to do to the instruction stream -- in the Linux build of this
;     same source, 145 -> 139 instructions, 26 -> 24 stores, 3 -> 0
;     vextracti128 -- and it is 2.55% SLOWER: 73.61 -> 71.73 MB/s, medians of
;     best-of-6 over 8 round-robin runs, with a COPY OF THE BASELINE BINARY in
;     the rotation reading -0.03%.  Neutral on AVX-512, where there is no
;     spill to relieve.  Default 0; rc_config.inc has the numbers and the
;     likely mechanism (the staged store's value now depends on the same
;     iteration's vpmulld rather than the previous one's).
;
;     So the spills are not simply free to remove, and "fewer uops always
;     wins" is not quite the whole model.  The accumulator knobs are no help
;     either -- RC_LOWSPLIT=0 costs the same two ymm per eight lanes that
;     lowl+lowh do, and RC_LOWBYTES below 7 makes the carry fallback fire
;     constantly.  Everything below is small change next to this.
;
;  2. `vmovdqa [rsp+..+var_150], ymm5` spills `range>>15` for lanes 0-7 and
;     `vpmulld ymm6, ymm6, [rsp+..+var_150]` reloads it as the multiply's
;     memory operand -- on that half's loop-carried chain.  Lanes 8-15 keep
;     the same value in a register (`vpmulld ymm10, ymm7, ymm10`).  2 uops,
;     and it is what makes the low half's chain ~25 cycles against the high
;     half's ~20.
;
;  3. Lane 15's store address is computed twice: `vextracti128 xmm12, ymm10,1`
;     + `vpextrd r8d, xmm12, 3` near the top, plus `add r8d, 0FFFFFFFEh` down
;     in the commit -- when `vpaddd ymm13, ymm10, [..var_170]` already
;     computes all eight of them and `vmovdqa [..var_310], ymm13` spills them.
;     3 uops, 2 of them p5, for what one `mov r8d, [..var_310+1Ch]` would do.
;
;  4. Both `vpbroadcastd` inside the loop rematerialise a constant every
;     group, and the -2 one re-spills it (`vmovdqa [..var_170], ymm11`) for a
;     single use 150 bytes later.  3 uops.  Same cause as 1.
;
;  5. TRIED, LOST.  The bit test costs 4 p5 uops per group -- 2 vpmovzxwd to
;     widen p and 2 vpmovsxwd to widen the bit mask -- because pbit is loaded
;     as unsigned words and widened twice.  One `vpmovsxwd ymm, m128` per
;     half should give both (`vpand 0x7fff` for p, `vpsrad 31` for the mask)
;     for 2 fewer p5 uops.  Reading pbit through a signed `short*` in the
;     sweep does NOT get clang there: it keeps the two widenings anyway and
;     picks up a vpcmpgtw as well.  Byte-identical stream, 71.80 vs 73.34
;     MB/s (medians of best-of-6 x 6 round-robin runs, 20 MB of enwik8),
;     i.e. 2.1% SLOWER.  It would need intrinsics, which the sweep is
;     deliberately not written in.
;
;  6. `add this,10h / cmp this,rdx / jb` is 2 fused uops; counting a negative
;     index up to zero makes it `add / jne` = 1.  pbit's [r14+rcx*2+102000h]
;     addressing still works off a biased base.  1 uop.
;
;  7. Not per-group, but: host_block is INLINED here, which is what produces
;     the 7B8h-byte frame, the `and rsp,-64`, the 10 xmm spills and the
;     ~55-entry address table at +0EEh.  None of it is on the vector path.
;     NOINLINE on host_block (the Linux build of the same source gets a call)
;     would roughly halve do_process for the I-cache and shorten the
;     prologue.  No effect on the loop itself.
;
;  ALREADY TRIED AND REJECTED, with numbers, in misc/avx2_profile.md:
;     branch past all-zero groups (RC_SCATTER_SKIP)  +12.07 cyc/grp
;     commit deferred one group                      +5.52
;     fusing pp into FSM                             +3.42
;     chunking the model pass and the sweep          +10.27
;     prefetching pbit                                +/-0 (noise)
;     -mprefer-vector-width=128                      much worse
;     RC_SHIFT_SAT=1 (lowl>>(32-sh))                 slower -- clang will not
;                                                    fold the poison away
;  Also a non-starter: trading vpmulld (2 uops, 10 cyc) for two vpmuludq plus
;  a recombine (3-4 uops).  Latency is not the constraint here, uop count is.
;=============================================================================
Model_0___do_process proc near          ; DATA XREF: processfile_0_+B6^o
                                        ; .pdata:00000001400BDB24^o

;-----------------------------------------------------------------------------
;  THE FRAME.  7B8h bytes, and most of it is the INLINED host_block:
;  var_3F8..var_668 are 55 quadwords of loop-invariant addresses for the 16
;  scalar rcC lanes (see +0EEh), and only the -C path reads them.
;
;  The vector lane state -- what rc_vecD.inc declares as local [RCNUM] arrays
;  -- is the ymmword block.  Two ymm per array, lanes 0-7 then 8-15:
;
;      var_3F0 / var_3D0   range[]        var_370 / var_350   lowl[]
;      var_2F0 / var_2D0   lowh[]         var_3B0 / var_390   rpre[]
;      var_6F0 / var_6D0   FFNum[]        var_2B0 / var_290   tmpptr[]
;      var_770 / var_750   stcl[]  -- RC_STAGE_CL's value  (== lowh, since
;                                     RC_LOWBYTES=8 makes cl the whole lowh)
;      var_330 / var_310   stad[]  -- RC_STAGE_CL's dword offset (tmpptr-2)
;
;  The sweep keeps private working copies in a second set of slots
;  (var_710/var_250 for tmpptr, var_110/var_690 for FFNum, plus var_130,
;  var_150 and the rotating var_1D0) and writes the canonical ones back at
;  the loop exit.  Broadcast constants: var_1F0 = 00ffffffh (sTOP-1),
;  var_210 = 0000ffffh (gTOP-1), var_230 = 16, var_170 = -2,
;  var_190 = 7fffc000h and var_1B0 = 80003fffh (the hSCALE header bit).
;
;  ALIGN(VECSIZE) on those arrays is what forces the `and rsp,-64` below,
;  which forces a frame pointer and takes rbp out of the allocator for the
;  whole function.  That cost is exactly why model_pass is NOINLINE.
;-----------------------------------------------------------------------------
C               = qword ptr -7A8h
var_7A0         = qword ptr -7A0h
var_798         = qword ptr -798h
var_790         = qword ptr -790h
var_788         = qword ptr -788h
var_780         = qword ptr -780h
blksize         = dword ptr -778h
var_770         = ymmword ptr -770h
var_750         = ymmword ptr -750h
var_710         = ymmword ptr -710h
var_6F0         = ymmword ptr -6F0h
var_6D0         = ymmword ptr -6D0h
var_690         = ymmword ptr -690h
var_670         = qword ptr -670h
var_668         = qword ptr -668h
var_660         = qword ptr -660h
var_658         = qword ptr -658h
var_650         = qword ptr -650h
var_648         = qword ptr -648h
var_640         = qword ptr -640h
var_638         = qword ptr -638h
var_630         = qword ptr -630h
var_628         = qword ptr -628h
var_620         = qword ptr -620h
var_618         = qword ptr -618h
var_610         = qword ptr -610h
var_608         = qword ptr -608h
var_600         = qword ptr -600h
var_5F8         = qword ptr -5F8h
var_5F0         = qword ptr -5F0h
var_5E8         = qword ptr -5E8h
var_5E0         = qword ptr -5E0h
var_5D8         = qword ptr -5D8h
var_5D0         = qword ptr -5D0h
var_5C8         = qword ptr -5C8h
var_5C0         = qword ptr -5C0h
var_5B8         = qword ptr -5B8h
var_5B0         = qword ptr -5B0h
var_5A8         = qword ptr -5A8h
var_5A0         = qword ptr -5A0h
var_598         = qword ptr -598h
var_590         = qword ptr -590h
var_588         = qword ptr -588h
var_580         = qword ptr -580h
var_578         = qword ptr -578h
var_570         = qword ptr -570h
var_568         = qword ptr -568h
var_560         = qword ptr -560h
var_558         = qword ptr -558h
var_550         = qword ptr -550h
var_548         = qword ptr -548h
var_540         = qword ptr -540h
var_538         = qword ptr -538h
var_530         = qword ptr -530h
var_528         = qword ptr -528h
var_520         = qword ptr -520h
var_518         = qword ptr -518h
var_510         = qword ptr -510h
var_508         = qword ptr -508h
var_500         = qword ptr -500h
var_4F8         = qword ptr -4F8h
var_4F0         = qword ptr -4F0h
var_4E8         = qword ptr -4E8h
var_4E0         = qword ptr -4E0h
var_4D8         = qword ptr -4D8h
var_4D0         = qword ptr -4D0h
var_4C8         = qword ptr -4C8h
var_4C0         = qword ptr -4C0h
var_4B8         = qword ptr -4B8h
var_4B0         = qword ptr -4B0h
var_4A8         = qword ptr -4A8h
var_4A0         = qword ptr -4A0h
var_498         = qword ptr -498h
var_490         = qword ptr -490h
var_488         = qword ptr -488h
var_480         = qword ptr -480h
var_478         = qword ptr -478h
var_470         = qword ptr -470h
var_468         = qword ptr -468h
var_460         = qword ptr -460h
var_458         = qword ptr -458h
var_450         = qword ptr -450h
var_448         = qword ptr -448h
var_440         = qword ptr -440h
var_438         = qword ptr -438h
var_430         = qword ptr -430h
var_428         = qword ptr -428h
var_420         = qword ptr -420h
var_418         = qword ptr -418h
var_410         = qword ptr -410h
var_408         = qword ptr -408h
var_400         = qword ptr -400h
var_3F8         = qword ptr -3F8h
var_3F0         = ymmword ptr -3F0h
var_3D0         = ymmword ptr -3D0h
var_3B0         = ymmword ptr -3B0h
var_390         = ymmword ptr -390h
var_370         = ymmword ptr -370h
var_350         = ymmword ptr -350h
var_330         = ymmword ptr -330h
var_310         = ymmword ptr -310h
var_2F0         = ymmword ptr -2F0h
var_2D0         = ymmword ptr -2D0h
var_2B0         = ymmword ptr -2B0h
var_290         = ymmword ptr -290h
var_250         = ymmword ptr -250h
var_230         = ymmword ptr -230h
var_210         = ymmword ptr -210h
var_1F0         = ymmword ptr -1F0h
var_1D0         = ymmword ptr -1D0h
var_1B0         = ymmword ptr -1B0h
var_190         = ymmword ptr -190h
var_170         = ymmword ptr -170h
var_150         = ymmword ptr -150h
var_130         = ymmword ptr -130h
var_110         = ymmword ptr -110h
var_E0          = xmmword ptr -0E0h
var_D0          = xmmword ptr -0D0h
var_C0          = xmmword ptr -0C0h
var_B0          = xmmword ptr -0B0h
var_A0          = xmmword ptr -0A0h
var_90          = xmmword ptr -90h
var_80          = xmmword ptr -80h
var_70          = xmmword ptr -70h
var_60          = xmmword ptr -60h
var_50          = xmmword ptr -50h

this = rcx
;=============================================================================
;  PROLOGUE
;=============================================================================
                push    rbp
                push    r15
                push    r14
                push    r13
                push    r12
                push    rsi
                push    rdi
                push    rbx
                sub     rsp, 7B8h                       ; 7B8h of frame -- see the note above the variable list
                lea     rbp, [rsp+80h]                  ; frame pointer: the realignment below needs one
                vmovaps [rbp+770h+var_50], xmm15        ; the MS x64 ABI makes xmm6-15 callee-saved: 10 spills
                vmovdqa [rbp+770h+var_60], xmm14
                vmovdqa [rbp+770h+var_70], xmm13
                vmovdqa [rbp+770h+var_80], xmm12
                vmovdqa [rbp+770h+var_90], xmm11
                vmovaps [rbp+770h+var_A0], xmm10
                vmovdqa [rbp+770h+var_B0], xmm9
                vmovdqa [rbp+770h+var_C0], xmm8
                vmovdqa [rbp+770h+var_D0], xmm7
                vmovdqa [rbp+770h+var_E0], xmm6
                and     rsp, 0FFFFFFFFFFFFFFC0h         ; 64-byte realign, forced by ALIGN(VECSIZE) on the lane arrays
                mov     r14, this                       ; r14 = this, for the whole function
                vxorps  xmm0, xmm0, xmm0
                vmovaps ymmword ptr [this+10320h], ymm0 ; \ P_Init(stats,DIM(stats)): 512 bytes of Counter state,
                vmovaps ymmword ptr [this+10300h], ymm0
                vmovaps ymmword ptr [this+102E0h], ymm0
                vmovaps ymmword ptr [this+102C0h], ymm0
                vmovaps ymmword ptr [this+102A0h], ymm0
                vmovaps ymmword ptr [this+10280h], ymm0
                vmovaps ymmword ptr [this+10260h], ymm0
                vmovaps ymmword ptr [this+10240h], ymm0
                vmovaps ymmword ptr [this+10220h], ymm0
                vmovaps ymmword ptr [this+10200h], ymm0
                vmovaps ymmword ptr [this+101E0h], ymm0
                vmovaps ymmword ptr [this+101C0h], ymm0
                vmovaps ymmword ptr [this+101A0h], ymm0
                vmovaps ymmword ptr [this+10180h], ymm0
                vmovaps ymmword ptr [this+10160h], ymm0
                vmovaps ymmword ptr [this+10140h], ymm0 ; / i.e. the order-0 context table
                mov     dword ptr [this+10100h], 1      ; ctx = 1
                lea     rax, [this+1013Eh]              ; \ inpptr = the coroutine's input cursor
                mov     [this+10108h], rax              ; /
                lea     r13, [this+11000h]              ; r13 = tmpbase = rcio.rows().  THE base of every payload store
                lea     rax, [this+102000h]             ; \ &pbit[0] -- the model pass's output, 1 MB, 4K-aligned
                mov     [rsp+7F0h+var_790], rax         ; /
                lea     rax, [this+202000h]             ; \ &res.lens[0] -- what rc_Write takes
                mov     [rsp+7F0h+var_670], rax         ; /
                lea     rax, [this+202040h]             ; \ &rcC.lane[0] -- the -C scalar coder, 64 bytes per lane
                mov     [rsp+7F0h+var_780], rax         ; /
;-----------------------------------------------------------------------------
;  +0EEh  LOOP-INVARIANT ADDRESS TABLE FOR THE INLINED host_block
;
;  55 lea/mov pairs holding, for each of the 16 scalar lanes:
;      ptr  = tmpbase + (j+1)*rowsize - 1     (rowsize = F0ACh = 61612)
;      beg  = ptr - RC_SKIP                   (RC_SKIP = 4)
;      end  = beg - tmpbufsize
;      &lane and &lane.low
;  Hoisted out of the block loop, so it runs once per do_process call and
;  costs nothing measurable.  It is still ~110 instructions and ~440 bytes of
;  frame that exist only because host_block got inlined -- improvement 7.
;-----------------------------------------------------------------------------
                lea     rax, [this+200ABh]
                mov     [rsp+7F0h+var_3F8], rax
                lea     rax, [this+200A7h]
                mov     [rsp+7F0h+var_400], rax
                lea     rax, [this+11040h]
                mov     [rsp+7F0h+var_408], rax
                lea     rax, [this+202060h]
                mov     [rsp+7F0h+var_410], rax
                lea     rax, [this+202080h]
                mov     [rsp+7F0h+var_418], rax
                lea     rax, [this+2F157h]
                mov     [rsp+7F0h+var_420], rax
                lea     rax, [this+2F153h]
                mov     [rsp+7F0h+var_428], rax
                lea     rax, [this+200ECh]
                mov     [rsp+7F0h+var_430], rax
;  --- and the sweep's broadcast constants, also hoisted out of the loop ---
                vxorps  xmm10, xmm10, xmm10             ; xmm10 = 0, for the rcC lane zeroing below
                vbroadcastss ymm0, cs:__real@00ffffff   ; sTOP-1
                vmovaps [rsp+7F0h+var_1F0], ymm0
                vbroadcastss ymm0, cs:__real@0000ffff   ; gTOP-1
                vmovaps [rsp+7F0h+var_210], ymm0
                vbroadcastss ymm0, cs:__real@00000010   ; 16 -- the `16-sh` of ShiftLow's split shift
                vmovaps [rsp+7F0h+var_230], ymm0
                vbroadcastss ymm0, cs:__real@7fffc000   ; (range>>15)*hSCALE for range = -1: the flag bit's rnew
                vmovaps [rsp+7F0h+var_190], ymm0
                vpbroadcastd ymm0, cs:__real@80003fff   ; and 80003fffh = range-rnew, the other side of it
                vmovdqa [rsp+7F0h+var_1B0], ymm0
                lea     rax, [this+2020A0h]
                mov     [rsp+7F0h+var_438], rax
                lea     rax, [this+2020C0h]
                mov     [rsp+7F0h+var_440], rax
                lea     rax, [this+3E203h]
                mov     [rsp+7F0h+var_448], rax
                lea     rax, [this+3E1FFh]
                mov     [rsp+7F0h+var_450], rax
                lea     rax, [this+2F198h]
                mov     [rsp+7F0h+var_458], rax
                lea     rax, [this+2020E0h]
                mov     [rsp+7F0h+var_460], rax
                lea     rax, [this+202100h]
                mov     [rsp+7F0h+var_468], rax
                lea     rax, [this+4D2AFh]
                mov     [rsp+7F0h+var_470], rax
                lea     rax, [this+4D2ABh]
                mov     [rsp+7F0h+var_478], rax
                lea     rax, [this+3E244h]
                mov     [rsp+7F0h+var_480], rax
                lea     rax, [this+202120h]
                mov     [rsp+7F0h+var_488], rax
                lea     rax, [this+202140h]
                mov     [rsp+7F0h+var_490], rax
                lea     rax, [this+5C35Bh]
                mov     [rsp+7F0h+var_498], rax
                lea     rax, [this+5C357h]
                mov     [rsp+7F0h+var_4A0], rax
                lea     rax, [this+4D2F0h]
                mov     [rsp+7F0h+var_4A8], rax
                lea     rax, [this+202160h]
                mov     [rsp+7F0h+var_4B0], rax
                lea     rax, [this+202180h]
                mov     [rsp+7F0h+var_4B8], rax
                lea     rax, [this+6B407h]
                mov     [rsp+7F0h+var_4C0], rax
                lea     rax, [this+6B403h]
                mov     [rsp+7F0h+var_4C8], rax
                lea     rax, [this+5C39Ch]
                mov     [rsp+7F0h+var_4D0], rax
                lea     rax, [this+2021A0h]
                mov     [rsp+7F0h+var_4D8], rax
                lea     rax, [this+2021C0h]
                mov     [rsp+7F0h+var_4E0], rax
                lea     rax, [this+7A4B3h]
                mov     [rsp+7F0h+var_4E8], rax
                lea     rax, [this+7A4AFh]
                mov     [rsp+7F0h+var_4F0], rax
                lea     rax, [this+6B448h]
                mov     [rsp+7F0h+var_4F8], rax
                lea     rax, [this+2021E0h]
                mov     [rsp+7F0h+var_500], rax
                lea     rax, [this+202200h]
                mov     [rsp+7F0h+var_508], rax
                lea     rax, [this+8955Fh]
                mov     [rsp+7F0h+var_510], rax
                lea     rax, [this+8955Bh]
                mov     [rsp+7F0h+var_518], rax
                lea     rax, [this+7A4F4h]
                mov     [rsp+7F0h+var_520], rax
                lea     rax, [this+202220h]
                mov     [rsp+7F0h+var_528], rax
                lea     rax, [this+202240h]
                mov     [rsp+7F0h+var_530], rax
                lea     rax, [this+9860Bh]
                mov     [rsp+7F0h+var_538], rax
                lea     rax, [this+98607h]
                mov     [rsp+7F0h+var_540], rax
                lea     rax, [this+895A0h]
                mov     [rsp+7F0h+var_548], rax
                lea     rax, [this+202260h]
                mov     [rsp+7F0h+var_550], rax
                lea     rax, [this+202280h]
                mov     [rsp+7F0h+var_558], rax
                lea     rax, [this+0A76B7h]
                mov     [rsp+7F0h+var_560], rax
                lea     rax, [this+0A76B3h]
                mov     [rsp+7F0h+var_568], rax
                lea     rax, [this+9864Ch]
                mov     [rsp+7F0h+var_570], rax
                lea     rax, [this+2022A0h]
                mov     [rsp+7F0h+var_578], rax
                lea     rax, [this+2022C0h]
                mov     [rsp+7F0h+var_580], rax
                lea     rax, [this+0B6763h]
                mov     [rsp+7F0h+var_588], rax
                lea     rax, [this+0B675Fh]
                mov     [rsp+7F0h+var_590], rax
                lea     rax, [this+0A76F8h]
                mov     [rsp+7F0h+var_598], rax
                lea     rax, [this+2022E0h]
                mov     [rsp+7F0h+var_5A0], rax
                lea     rax, [this+202300h]
                mov     [rsp+7F0h+var_5A8], rax
                lea     rax, [this+0C580Fh]
                mov     [rsp+7F0h+var_5B0], rax
                lea     rax, [this+0C580Bh]
                mov     [rsp+7F0h+var_5B8], rax
                lea     rax, [this+0B67A4h]
                mov     [rsp+7F0h+var_5C0], rax
                lea     rax, [this+202320h]
                mov     [rsp+7F0h+var_5C8], rax
                lea     rax, [this+202340h]
                mov     [rsp+7F0h+var_5D0], rax
                lea     rax, [this+0D48BBh]
                mov     [rsp+7F0h+var_5D8], rax
                lea     rax, [this+0D48B7h]
                mov     [rsp+7F0h+var_5E0], rax
                lea     rax, [this+0C5850h]
                mov     [rsp+7F0h+var_5E8], rax
                lea     rax, [this+202360h]
                mov     [rsp+7F0h+var_5F0], rax
                lea     rax, [this+202380h]
                mov     [rsp+7F0h+var_5F8], rax
                lea     rax, [this+0E3967h]
                mov     [rsp+7F0h+var_600], rax
                lea     rax, [this+0E3963h]
                mov     [rsp+7F0h+var_608], rax
                lea     rax, [this+0D48FCh]
                mov     [rsp+7F0h+var_610], rax
                lea     rax, [this+2023A0h]
                mov     [rsp+7F0h+var_618], rax
                lea     rax, [this+2023C0h]
                mov     [rsp+7F0h+var_620], rax
                lea     rax, [this+0F2A13h]
                mov     [rsp+7F0h+var_628], rax
                lea     rax, [this+0F2A0Fh]
                mov     [rsp+7F0h+var_630], rax
                lea     rax, [this+0E39A8h]
                mov     [rsp+7F0h+var_638], rax
                lea     rax, [this+2023E0h]
                mov     [rsp+7F0h+var_640], rax
                lea     rax, [this+202400h]
                mov     [rsp+7F0h+var_648], rax
                lea     rax, [this+101ABFh]
                mov     [rsp+7F0h+var_650], rax
                lea     rax, [this+101ABBh]
                mov     [rsp+7F0h+var_658], rax
                lea     rax, [this+0F2A54h]
                mov     [rsp+7F0h+var_660], rax
                lea     rax, [this+202420h]
                mov     [rsp+7F0h+var_668], rax
;  --- the block loop's own invariants ---
                lea     rsi, [this+202078h]             ; rsi = &rcC.lane[0].low, the -C loop's base
                vmovq   xmm15, cs:__xmm@00000000000000000001020304050607  ; byte-reverse table for rc_Quit's 8-byte flush
                mov     qword ptr [rsp+7F0h+blksize], this
                mov     [rsp+7F0h+C], r13               ; [C] = tmpbase, reloaded wherever r13 gets clobbered
                mov     [rsp+7F0h+var_798], rsi
                jmp     short loc_1400C0A40
; ---------------------------------------------------------------------------
                align 20h

;=============================================================================
;  +660h  CARRY FALLBACK
;  Some lane let a carry escape the carryless coder, so the whole block is
;  re-coded with the carry-propagating twin.  At RC_LOWBYTES=8 that is a
;  ~2^-33 event per step: in practice this never runs.
;=============================================================================
loc_1400C0A20:                          ; CODE XREF: Model_0___do_process+229Dvj
                mov     this, r14       ; blksize
                mov     rdx, [rsp+7F0h+var_7A0]
                call    Model_0___recode_with_carry
                mov     rsi, [rsp+7F0h+var_798]

;=============================================================================
;  +672h  `if( blksize<BLKSIZE ) break;`  -- the do_process loop latch
;=============================================================================
loc_1400C0A32:                          ; CODE XREF: Model_0___do_process+2130vj
                                        ; Model_0___do_process+2145vj ...  ; var_788 = bytes left in the input buffer, before the min
                cmp     dword ptr [rsp+7F0h+var_788], 0FFFFh
                jbe     loc_1400C2814

;=============================================================================
;  +680h  BLOCK LOOP HEAD -- once per 65536 input bytes
;=============================================================================
loc_1400C0A40:                          ; CODE XREF: Model_0___do_process+65A^j
                mov     rax, [r14]                      ; \ chkinp(): inpptr vs inpend ...
                mov     rdi, [r14+10h]
                cmp     rax, rdi                        ; /
                jb      short loc_1400C0A6B
                movzx   eax, word ptr [r14+1Ch]         ; \ ... empty, so yield back to the coroutine for a refill
                mov     this, r14
                sub     this, rax       ; p
                movzx   edx, word ptr [r14+1Eh] ; value
                vzeroupper
                call    yield_0                         ; /
                mov     rax, [r14]
                mov     rdi, [r14+10h]

loc_1400C0A6B:                          ; CODE XREF: Model_0___do_process+68A^j
                sub     edi, eax                        ; \ blksize = min( available, BLKSIZE )
                cmp     edi, 10000h
                mov     edx, 10000h
                cmovb   edx, edi                        ; /
                mov     this, r14       ; blksize
                mov     [rsp+7F0h+var_7A0], rdx
                vzeroupper
                call    Model_0___model_pass            ; the model pass: 8 x {(bit<<15)+p} per input byte, into pbit[]
;  --- the -C dispatch.  `g_use_vec` reads INVERTED here: LTO narrowed the
;      int to a bool and flipped its polarity so the default value lands in
;      .bss, so a NONZERO byte means "-C was given, use the scalar
;      reference".  Falls through to host_block; jz takes the vector path.
                cmp     cs:byte_140088300, 0            ; -C given?
                mov     [rsp+7F0h+var_788], rdi
                jz      loc_1400C11F0                   ; no -> the vector coder at +E30h
;=============================================================================
;  +6DDh  host_block -- THE -C SCALAR REFERENCE PATH.  COLD: reached only
;  with -C, and t.sh compares its output against the vector path byte for
;  byte.  Skip to +E30h for the code that actually runs.
;
;  What follows is rcC.rc_Init(rcio) unrolled 16 times: per lane the three
;  RC_IO pointers, f_OVF=0 (+18h), the carry counter=0 (+38h), low and rpre
;  zeroed as one xmm (+20h), range=-1 (+34h) and Cache=-1 (+3Ch).
;=============================================================================
                mov     rax, [rsp+7F0h+var_3F8]
                mov     [r14+202048h], rax
                mov     rax, [rsp+7F0h+var_400]
                mov     [r14+202040h], rax
                mov     rax, [rsp+7F0h+var_408]
                mov     [r14+202050h], rax
                mov     dword ptr [r14+202058h], 0
                mov     dword ptr [r14+202078h], 0
                mov     rax, [rsp+7F0h+var_410]
                vmovaps xmmword ptr [rax], xmm10
                mov     dword ptr [r14+20207Ch], 0FFFFFFFFh
                mov     dword ptr [r14+202074h], 0FFFFFFFFh
                mov     rax, [rsp+7F0h+var_420]
                mov     [r14+202088h], rax
                mov     rax, [rsp+7F0h+var_428]
                mov     [r14+202080h], rax
                mov     rax, [rsp+7F0h+var_430]
                mov     [r14+202090h], rax
                mov     dword ptr [r14+202098h], 0
                mov     dword ptr [r14+2020B8h], 0
                mov     rax, [rsp+7F0h+var_438]
                vmovaps xmmword ptr [rax], xmm10
                mov     dword ptr [r14+2020BCh], 0FFFFFFFFh
                mov     dword ptr [r14+2020B4h], 0FFFFFFFFh
                mov     rax, [rsp+7F0h+var_448]
                mov     [r14+2020C8h], rax
                mov     rax, [rsp+7F0h+var_450]
                mov     [r14+2020C0h], rax
                mov     rax, [rsp+7F0h+var_458]
                mov     [r14+2020D0h], rax
                mov     dword ptr [r14+2020D8h], 0
                mov     dword ptr [r14+2020F8h], 0
                mov     rax, [rsp+7F0h+var_460]
                vmovaps xmmword ptr [rax], xmm10
                mov     dword ptr [r14+2020FCh], 0FFFFFFFFh
                mov     dword ptr [r14+2020F4h], 0FFFFFFFFh
                mov     rax, [rsp+7F0h+var_470]
                mov     [r14+202108h], rax
                mov     rax, [rsp+7F0h+var_478]
                mov     [r14+202100h], rax
                mov     rax, [rsp+7F0h+var_480]
                mov     [r14+202110h], rax
                mov     dword ptr [r14+202118h], 0
                mov     dword ptr [r14+202138h], 0
                mov     rax, [rsp+7F0h+var_488]
                vmovaps xmmword ptr [rax], xmm10
                mov     dword ptr [r14+20213Ch], 0FFFFFFFFh
                mov     dword ptr [r14+202134h], 0FFFFFFFFh
                mov     rax, [rsp+7F0h+var_498]
                mov     [r14+202148h], rax
                mov     rax, [rsp+7F0h+var_4A0]
                mov     [r14+202140h], rax
                mov     rax, [rsp+7F0h+var_4A8]
                mov     [r14+202150h], rax
                mov     dword ptr [r14+202158h], 0
                mov     dword ptr [r14+202178h], 0
                mov     rax, [rsp+7F0h+var_4B0]
                vmovaps xmmword ptr [rax], xmm10
                mov     dword ptr [r14+20217Ch], 0FFFFFFFFh
                mov     dword ptr [r14+202174h], 0FFFFFFFFh
                mov     rax, [rsp+7F0h+var_4C0]
                mov     [r14+202188h], rax
                mov     rax, [rsp+7F0h+var_4C8]
                mov     [r14+202180h], rax
                mov     rax, [rsp+7F0h+var_4D0]
                mov     [r14+202190h], rax
                mov     dword ptr [r14+202198h], 0
                mov     dword ptr [r14+2021B8h], 0
                mov     rax, [rsp+7F0h+var_4D8]
                vmovaps xmmword ptr [rax], xmm10
                mov     dword ptr [r14+2021BCh], 0FFFFFFFFh
                mov     dword ptr [r14+2021B4h], 0FFFFFFFFh
                mov     rax, [rsp+7F0h+var_4E8]
                mov     [r14+2021C8h], rax
                mov     rax, [rsp+7F0h+var_4F0]
                mov     [r14+2021C0h], rax
                mov     rax, [rsp+7F0h+var_4F8]
                mov     [r14+2021D0h], rax
                mov     dword ptr [r14+2021D8h], 0
                mov     dword ptr [r14+2021F8h], 0
                mov     rax, [rsp+7F0h+var_500]
                vmovaps xmmword ptr [rax], xmm10
                mov     dword ptr [r14+2021FCh], 0FFFFFFFFh
                mov     dword ptr [r14+2021F4h], 0FFFFFFFFh
                mov     rax, [rsp+7F0h+var_510]
                mov     [r14+202208h], rax
                mov     rax, [rsp+7F0h+var_518]
                mov     [r14+202200h], rax
                mov     rax, [rsp+7F0h+var_520]
                mov     [r14+202210h], rax
                mov     dword ptr [r14+202218h], 0
                mov     dword ptr [r14+202238h], 0
                mov     rax, [rsp+7F0h+var_528]
                vmovaps xmmword ptr [rax], xmm10
                mov     dword ptr [r14+20223Ch], 0FFFFFFFFh
                mov     dword ptr [r14+202234h], 0FFFFFFFFh
                mov     rax, [rsp+7F0h+var_538]
                mov     [r14+202248h], rax
                mov     rax, [rsp+7F0h+var_540]
                mov     [r14+202240h], rax
                mov     rax, [rsp+7F0h+var_548]
                mov     [r14+202250h], rax
                mov     dword ptr [r14+202258h], 0
                mov     dword ptr [r14+202278h], 0
                mov     rax, [rsp+7F0h+var_550]
                vmovaps xmmword ptr [rax], xmm10
                mov     dword ptr [r14+20227Ch], 0FFFFFFFFh
                mov     dword ptr [r14+202274h], 0FFFFFFFFh
                mov     rax, [rsp+7F0h+var_560]
                mov     [r14+202288h], rax
                mov     rax, [rsp+7F0h+var_568]
                mov     [r14+202280h], rax
                mov     rax, [rsp+7F0h+var_570]
                mov     [r14+202290h], rax
                mov     dword ptr [r14+202298h], 0
                mov     dword ptr [r14+2022B8h], 0
                mov     rax, [rsp+7F0h+var_578]
                vmovaps xmmword ptr [rax], xmm10
                mov     dword ptr [r14+2022BCh], 0FFFFFFFFh
                mov     dword ptr [r14+2022B4h], 0FFFFFFFFh
                mov     rax, [rsp+7F0h+var_588]
                mov     [r14+2022C8h], rax
                mov     rax, [rsp+7F0h+var_590]
                mov     [r14+2022C0h], rax
                mov     rax, [rsp+7F0h+var_598]
                mov     [r14+2022D0h], rax
                mov     dword ptr [r14+2022D8h], 0
                mov     dword ptr [r14+2022F8h], 0
                mov     rax, [rsp+7F0h+var_5A0]
                vmovaps xmmword ptr [rax], xmm10
                mov     dword ptr [r14+2022FCh], 0FFFFFFFFh
                mov     dword ptr [r14+2022F4h], 0FFFFFFFFh
                mov     rax, [rsp+7F0h+var_5B0]
                mov     [r14+202308h], rax
                mov     rax, [rsp+7F0h+var_5B8]
                mov     [r14+202300h], rax
                mov     rax, [rsp+7F0h+var_5C0]
                mov     [r14+202310h], rax
                mov     dword ptr [r14+202318h], 0
                mov     dword ptr [r14+202338h], 0
                mov     rax, [rsp+7F0h+var_5C8]
                vmovaps xmmword ptr [rax], xmm10
                mov     dword ptr [r14+20233Ch], 0FFFFFFFFh
                mov     dword ptr [r14+202334h], 0FFFFFFFFh
                mov     rax, [rsp+7F0h+var_5D8]
                mov     [r14+202348h], rax
                mov     rax, [rsp+7F0h+var_5E0]
                mov     [r14+202340h], rax
                mov     rax, [rsp+7F0h+var_5E8]
                mov     [r14+202350h], rax
                mov     dword ptr [r14+202358h], 0
                mov     dword ptr [r14+202378h], 0
                mov     rax, [rsp+7F0h+var_5F0]
                vmovaps xmmword ptr [rax], xmm10
                mov     dword ptr [r14+20237Ch], 0FFFFFFFFh
                mov     dword ptr [r14+202374h], 0FFFFFFFFh
                mov     rax, [rsp+7F0h+var_600]
                mov     [r14+202388h], rax
                mov     rax, [rsp+7F0h+var_608]
                mov     [r14+202380h], rax
                mov     rax, [rsp+7F0h+var_610]
                mov     [r14+202390h], rax
                mov     dword ptr [r14+202398h], 0
                mov     dword ptr [r14+2023B8h], 0
                mov     rax, [rsp+7F0h+var_618]
                vmovaps xmmword ptr [rax], xmm10
                mov     dword ptr [r14+2023BCh], 0FFFFFFFFh
                mov     dword ptr [r14+2023B4h], 0FFFFFFFFh
                mov     rax, [rsp+7F0h+var_628]
                mov     [r14+2023C8h], rax
                mov     rax, [rsp+7F0h+var_630]
                mov     [r14+2023C0h], rax
                mov     rax, [rsp+7F0h+var_638]
                mov     [r14+2023D0h], rax
                mov     dword ptr [r14+2023D8h], 0
                mov     dword ptr [r14+2023F8h], 0
                mov     rax, [rsp+7F0h+var_640]
                vmovaps xmmword ptr [rax], xmm10
                mov     dword ptr [r14+2023FCh], 0FFFFFFFFh
                mov     dword ptr [r14+2023F4h], 0FFFFFFFFh
                mov     rax, [rsp+7F0h+var_650]
                mov     [r14+202408h], rax
                mov     rax, [rsp+7F0h+var_658]
                mov     [r14+202400h], rax
                mov     rax, [rsp+7F0h+var_660]
                mov     [r14+202410h], rax
                mov     dword ptr [r14+202418h], 0
                mov     dword ptr [r14+202438h], 0
                mov     rax, [rsp+7F0h+var_668]
                vmovaps xmmword ptr [rax], xmm10
                mov     dword ptr [r14+20243Ch], 0FFFFFFFFh
                mov     dword ptr [r14+202434h], 0FFFFFFFFh
;  --- proc_block: the block-length flag bit, rc_Process(0,hSCALE,bit),
;      coded scalar into lane 0, then ShiftLow's counted 0/1/2-byte renorm.
                mov     ecx, [r14+202074h]              ; range
                xor     eax, eax
                cmp     ecx, 1000000h
                setb    al
                mov     edx, [r14+202064h]              ; lowh
                mov     r8d, [r14+202068h]              ; rpre
                add     [r14+202060h], r8d              ; low += rpre, 64-bit, carry out into the counter at +38h
                adc     edx, 0
                adc     dword ptr [r14+202078h], 0      ; the lost-carry counter that rc_Carries() reads
                cmp     ecx, 10000h
                adc     eax, 0
                mov     [r14+202064h], edx
                jz      short loc_1400C11AB
                cmp     ecx, 10000h
                setb    r8b
                cmp     ecx, 1000000h
                setb    r9b
                shr     edx, 18h                        ; \ ShiftLow: emit low's top byte, ptr--, low <<= 8
                mov     this, [r14+202048h]
                lea     r10, [this-1]
                mov     [r14+202048h], r10
                mov     [this], dl
                mov     ecx, [r14+202063h]
                mov     [r14+202064h], ecx
                shl     dword ptr [r14+202060h], 8      ; /
                xor     r9b, r8b
                jnz     short loc_1400C11A4
                shr     ecx, 18h
                mov     rdx, [r14+202048h]
                lea     r8, [rdx-1]
                mov     [r14+202048h], r8
                mov     [rdx], cl
                mov     ecx, [r14+202063h]
                mov     [r14+202064h], ecx
                shl     dword ptr [r14+202060h], 8

loc_1400C11A4:                          ; CODE XREF: Model_0___do_process+DB5^j
                mov     ecx, [r14+202074h]

loc_1400C11AB:                          ; CODE XREF: Model_0___do_process+D6F^j
                mov     rdx, [rsp+7F0h+var_788]
                shl     eax, 3                          ; sh = n*8
                shlx    ecx, ecx, eax                   ; range <<= sh
                mov     eax, ecx
                shr     eax, 1
                and     eax, 7FFFC000h                  ; rnew = (range>>15)*hSCALE, i.e. (range>>1) & 7fffc000h
                cmp     edx, 10000h
                jb      loc_1400C1A30                   ; short block -> code the 16-bit length header too
                mov     [r14+202074h], eax
                mov     dword ptr [r14+202068h], 0
                mov     ecx, 80000h
                mov     eax, 80000h
                jmp     loc_1400C1B7A
; ---------------------------------------------------------------------------
                align 10h

;=============================================================================
;  +E30h  THE VECTOR PATH -- per-block init
;
;    for( j=0; j<RCNUM; j++ ) { tmpptr[j] = (j+1)*rowsize-2; rc_Init(j); }
;
;  All 16 lanes, two ymm each, straight into the frame slots.
;=============================================================================
loc_1400C11F0:                          ; CODE XREF: Model_0___do_process+6D7^j
                vpxor   xmm9, xmm9, xmm9
                vmovdqa [rsp+7F0h+var_370], ymm9        ; \ lowl[] = 0, lowh[] = 0, rpre[] = 0
                vmovdqa [rsp+7F0h+var_350], ymm9
                vmovdqa [rsp+7F0h+var_2F0], ymm9
                vmovdqa [rsp+7F0h+var_2D0], ymm9
                vmovdqa [rsp+7F0h+var_3B0], ymm9
                vmovdqa [rsp+7F0h+var_390], ymm9        ; /
                vpcmpeqd ymm1, ymm1, ymm1               ; \ FFNum[] = -1 (the no-carry MASK) and range[] = -1
                vmovdqa [rsp+7F0h+var_6F0], ymm1
                vmovdqa [rsp+7F0h+var_6D0], ymm1
                vmovdqa [rsp+7F0h+var_3F0], ymm1
                vmovdqa [rsp+7F0h+var_3D0], ymm1        ; /
                vmovdqa [rsp+7F0h+var_770], ymm9        ; \ stcl[] = 0  -- RC_STAGE_INIT's value
                vmovdqa [rsp+7F0h+var_750], ymm9        ; /
                vmovaps ymm0, cs:__ymm@000f0abe000e1a12000d2966000c38ba000b480e000a5762000966b60008760a  ; tmpptr[8..15] = (j+1)*F0ACh-2, lanes 15..8 high dword to low
                vmovaps [rsp+7F0h+var_290], ymm0
                vmovdqa ymm0, cs:__ymm@000f0abc000e1a10000d2964000c38b8000b480c000a5760000966b400087608  ; stad[8..15] = tmpptr-2  -- RC_STAGE_INIT's seed
                vmovdqa [rsp+7F0h+var_310], ymm0
                vpxor   xmm6, xmm6, xmm6                ; ymm6 = rpre[8..15] = 0
                vmovaps ymm3, cs:__ymm@0007855e000694b20005a4060004b35a0003c2ae0002d2020001e1560000f0aa  ; tmpptr[0..7]
                vmovaps [rsp+7F0h+var_710], ymm3
                cmp     edi, 10000h                     ; bit = (blksize!=BLKSIZE), the block-length flag
                mov     eax, 7FFFC000h
                mov     ecx, 80003FFFh
                cmovb   eax, ecx                        ; range[0] = bit ? 80003fffh : 7fffc000h   -- fully const-folded
                mov     ecx, 0
                mov     edx, 7FFFC000h
                cmovb   ecx, edx                        ; rpre[0]  = bit ? 7fffc000h : 0
                vmovaps ymm2, cs:__ymm@0007855c000694b00005a4040004b3580003c2ac0002d2000001e1540000f0a8  ; stad[0..7]
                vmovaps [rsp+7F0h+var_330], ymm2
                vmovaps [rsp+7F0h+var_2B0], ymm3        ; the canonical tmpptr[0..7] slot
                mov     dword ptr [rsp+7F0h+var_370], 0 ; lane 0 only: the other 15 keep their init values
                mov     dword ptr [rsp+7F0h+var_3F0], eax
                mov     dword ptr [rsp+7F0h+var_3B0], ecx
                mov     dword ptr [r14+200A8h], 0       ; RC_FLUSH_ONE(0): tmpbase[stad[0]] = cl = 0, address constant
                vmovdqa ymm11, [rsp+7F0h+var_1F0]       ; \ the four constants the sweep wants in registers
                vmovdqa ymm12, [rsp+7F0h+var_210]
                vpcmpeqd ymm13, ymm13, ymm13
                vmovdqa ymm14, [rsp+7F0h+var_230]       ; /
                jnb     loc_1400C1539                   ; not a short block -> no length header, straight to the sweep
;  --- the 16-bit block length, ONLY for the final short block of a file.
;      The source loops j=0..15 coding bit (blksize>>(15-j)) into lane j, so
;      every lane takes exactly ONE step -- which is why clang could unroll
;      it and re-vectorise it as a single 16-lane rc_Process + ShiftLow.
;      Runs once per FILE, never in steady state.
                vmovdqa ymm0, [rsp+7F0h+var_3F0]
                vpminud ymm1, ymm0, ymm11               ; \ n = (range<sTOP) + (range<gTOP), the AVX2 way:
                vpcmpeqd ymm1, ymm0, ymm1
                vpsrld  ymm2, ymm1, 1Fh
                vpminud ymm1, ymm0, ymm12               ; /  vpminud + vpcmpeqd, twice, since there is no vpcmpud
                vmovdqa ymm3, [rsp+7F0h+var_370]
                vpaddd  ymm4, ymm3, [rsp+7F0h+var_3B0]  ; lowl += rpre
                vpcmpeqd ymm5, ymm0, ymm1
                vpmaxud ymm1, ymm4, ymm3                ; \ carry out of lowl, then out of lowh
                vpcmpeqd ymm1, ymm4, ymm1
                vmovdqa ymm3, [rsp+7F0h+var_2F0]
                vpaddd  ymm1, ymm3, ymm1
                vpsubd  ymm1, ymm1, ymm13
                vpmaxud ymm3, ymm1, ymm3
                vpcmpeqd ymm3, ymm1, ymm3               ; /
                vpxor   ymm3, ymm13, ymm3
                vpmaskmovd [rsp+7F0h+var_6F0], ymm3, ymm9  ; FFNum &= _cy-1, as a masked store of zero
                vpsubd  ymm2, ymm2, ymm5
                vpslld  ymm3, ymm2, 3
                vpsrld  ymm5, ymm4, 10h                 ; \ _hi = (lowl>>16) >> (16-sh)
                vpsubd  ymm6, ymm14, ymm3
                vpsrlvd ymm5, ymm5, ymm6                ; /
                vpsllvd ymm6, ymm1, ymm3
                vpor    ymm5, ymm6, ymm5                ; lowh = (lowh<<sh) | _hi
                vmovdqa [rsp+7F0h+var_2F0], ymm5
                vmovd   xmm5, dword ptr [rsp+7F0h+var_7A0]
                vpbroadcastd ymm5, xmm5                 ; broadcast blksize
                vpsllvd ymm4, ymm4, ymm3
                vmovdqa [rsp+7F0h+var_370], ymm4
                vpsllvd ymm0, ymm0, ymm3
                vpsrld  ymm3, ymm0, 1                   ; rnew = (range>>15)*hSCALE
                vmovdqa ymm6, [rsp+7F0h+var_190]
                vpand   ymm3, ymm3, ymm6
                vpsubd  ymm0, ymm0, ymm3
                vpand   ymm4, ymm5, cs:__ymm@0000010000000200000004000000080000001000000020000000400000008000  ; \ bit = (blksize>>(15-j))&1 for j=0..7, via {8000h..100h}
                vpcmpeqd ymm4, ymm9, ymm4               ; /  mask is ALL-ONES when the bit is ZERO
                vblendvps ymm0, ymm0, ymm3, ymm4        ; range = bit ? range-rnew : rnew
                vmovaps [rsp+7F0h+var_3F0], ymm0
                vandnps ymm0, ymm4, ymm3                ; rpre  = bit ? rnew : 0
                vmovaps [rsp+7F0h+var_3B0], ymm0
                vmovdqa [rsp+7F0h+var_770], ymm1        ; stcl[0..7] = lowh
                vmovdqa ymm0, cs:__ymm@0007855e000694b20005a4060004b35a0003c2ae0002d2020001e1560000f0aa
                vpsubd  ymm0, ymm0, ymm2                ; tmpptr -= n
                vmovdqa [rsp+7F0h+var_710], ymm0
                vmovdqa [rsp+7F0h+var_2B0], ymm0
                vmovd   dword ptr [r14+200A8h], xmm1    ; \ RC_FLUSH_ONE x8, with fully CONSTANT addresses -- 200A8h =
                vpextrd dword ptr [r14+2F154h], xmm1, 1
                vpextrd dword ptr [r14+3E200h], xmm1, 2
                vpextrd dword ptr [r14+4D2ACh], xmm1, 3
                vextracti128 xmm1, ymm1, 1
                vmovd   dword ptr [r14+5C358h], xmm1
                vpextrd dword ptr [r14+6B404h], xmm1, 1
                vpextrd dword ptr [r14+7A4B0h], xmm1, 2
                vpextrd dword ptr [r14+8955Ch], xmm1, 3 ; /  tmpbase + 1*rowsize - 2, and F0ACh apart after that
                vmovdqa [rsp+7F0h+var_750], ymm9
                vmovaps ymm1, cs:__ymm@000f0abe000e1a12000d2966000c38ba000b480e000a5762000966b60008760a
                vmovaps [rsp+7F0h+var_290], ymm1
                vmovdqa [rsp+7F0h+var_2D0], ymm9
                vmovdqa [rsp+7F0h+var_350], ymm9
                vpand   ymm1, ymm5, cs:__ymm@0000000100000002000000040000000800000010000000200000004000000080  ; the low half of the length, {80h..1h}, for lanes 8..15
                vpcmpeqd ymm2, ymm9, ymm1
                vmovaps ymm1, [rsp+7F0h+var_1B0]
                vblendvps ymm1, ymm1, ymm6, ymm2
                vmovaps [rsp+7F0h+var_3D0], ymm1
                vpandn  ymm6, ymm2, ymm6
                vmovdqa [rsp+7F0h+var_390], ymm6
                mov     dword ptr [r14+98608h], 0       ; \ RC_FLUSH_ONE for lanes 8..15: cl is still 0 in all of them,
                mov     dword ptr [r14+0A76B4h], 0
                mov     dword ptr [r14+0B6760h], 0
                mov     dword ptr [r14+0C580Ch], 0
                mov     dword ptr [r14+0D48B8h], 0
                mov     dword ptr [r14+0E3964h], 0
                mov     dword ptr [r14+0F2A10h], 0
                mov     dword ptr [r14+101ABCh], 0      ; /  so this is eight immediate stores

;=============================================================================
;  +1179h  SWEEP PREAMBLE
;      blksize0 = blksize*8;  blksize1 = blksize0 - blksize0%RCNUM;
;  Lane state is hoisted out of the frame slots into ymm for the loop.
;=============================================================================
loc_1400C1539:                          ; CODE XREF: Model_0___do_process+F5A^j
                mov     rax, [rsp+7F0h+var_7A0]
                lea     eax, ds:0[rax*8]                ; blksize0 = blksize*8
                mov     ecx, eax
                and     ecx, 0FFFF0h                    ; blksize1 -- and skip the sweep entirely if it is empty
                jz      loc_1400C1DB5
                mov     edx, ecx
                vmovdqa ymm0, [rsp+7F0h+var_370]        ; ymm0  = lowl[0..7]
                vmovdqa ymm7, [rsp+7F0h+var_3B0]        ; ymm7  = rpre[0..7]
                vmovdqa ymm8, [rsp+7F0h+var_2F0]        ; ymm8  = lowh[0..7]
                vmovdqa ymm10, [rsp+7F0h+var_6F0]       ; ymm10 = FFNum[0..7]
                vpxor   xmm4, xmm4, xmm4                ; lowh[8..15] = 0 on both paths in: one header step cannot
                vpcmpeqd ymm2, ymm2, ymm2
                vmovdqa [rsp+7F0h+var_690], ymm2        ; the sweep's private FFNum[8..15]: stays in memory all loop
                vmovdqa ymm14, [rsp+7F0h+var_3F0]       ; ymm14 = range[0..7]
                vmovaps ymm3, cs:__ymm@000f0abe000e1a12000d2966000c38ba000b480e000a5762000966b60008760a  ; tmpptr[8..15], rematerialised as the same CONSTANT it was
                vmovaps [rsp+7F0h+var_250], ymm3        ; initialised to: no header step can have moved it
                vpxor   xmm2, xmm2, xmm2                ; renormalise, so lowl/lowh/tmpptr are still at their init
                xor     ecx, ecx                        ; rcx = i, the bit index; rdx = blksize1
                nop     word ptr [rax+rax+00h]

;=============================================================================
;=============================================================================
;  +11F0h   ****  THE HOT LOOP  ****    one pass = one group = 16 coded bits
;
;    for( i=0; i<blksize1; i+=RCNUM ) {
;      for( j=0; j<RCNUM; j++ ) {                 // RC_UNROLL, fully unrolled
;        uint b = pbit[i+j]; uint f = b&0x7FFF; b >>= 15;
;        rc_Process( j, f, b );                   // ShiftLow + the coding step
;      }
;      RC_FLUSHALL();                             // 16 staged stores commit
;    }
;
;  RCNUM=16 lanes = two ymm per array, and the two halves are two independent
;  dependency chains.  L = lanes 0-7, H = lanes 8-15.  Live across the
;  backedge: ymm0 lowl_L, ymm7 rpre_L, ymm8 lowh_L, ymm14 range_L, ymm10
;  FFNum_L, ymm2 lowl_H, ymm6 rpre_H, ymm4 lowh_H, ymm1 range_H, ymm15
;  stcl_L, ymm13 stad_H, plus tmpptr, FFNum_H and the staging arrays in the
;  frame.  rcx = i, rdx = blksize1, r14 = this, +11000h = tmpbase.
;
;  145 instructions, ~158 fused uops, 29 stores, 30 loads, 38.06 cycles.
;  Rename-bound; see the notes at the top of the file.
;=============================================================================
;=============================================================================
loc_1400C15B0:                          ; CODE XREF: Model_0___do_process+1595vj
                vmovdqa [rsp+7F0h+var_1D0], ymm6        ; spill rpre_H -- this slot gets reused for lowh_L below
                vpaddd  ymm5, ymm7, ymm0                ; L: lowl += rpre               <-- ShiftLow (low_Add)
                vpmaxud ymm6, ymm5, ymm0                ; \ no vpcmpud on AVX2: max(sum,lowl)==sum iff no carry.
                vpcmpeqd ymm6, ymm5, ymm6               ; /  _cf mask, ALL-ONES when there was NO carry out of lowl
                vpaddd  ymm6, ymm8, ymm6                ; \ lowh += _cf, in two adds because the mask is 0/-1:
                vpcmpeqd ymm0, ymm0, ymm0               ;  |   lowh + mask  ... then +1
                vpsubd  ymm15, ymm6, ymm0               ; /
                vpmaxud ymm6, ymm15, ymm8               ; \ the same trick again, for the carry OUT of lowh
                vmovdqa [rsp+7F0h+var_770], ymm15       ; stcl[0..7] = lowh   <-- STAGING STORE (cl == lowh at LOWBYTES=8)
                vpcmpeqd ymm6, ymm15, ymm6              ; /  _cy-1 mask: all-ones = this lane kept its carry
                vpbroadcastd ymm11, cs:__real@fffffffe  ; the constant -2, rematerialised every group ...
                vmovdqa [rsp+7F0h+var_170], ymm11       ; ... and re-spilled for one use 150 bytes down.  Improvement 4
                vmovdqa ymm0, [rsp+7F0h+var_1F0]        ; sTOP-1
                vpminud ymm7, ymm14, ymm0               ; \ range <= 00ffffffh ?
                vpcmpeqd ymm7, ymm14, ymm7              ; /
                vpsrld  ymm7, ymm7, 1Fh                 ; mask -> 0/1
                vmovdqa ymm3, [rsp+7F0h+var_210]        ; gTOP-1
                vpminud ymm8, ymm14, ymm3               ; \ range <= 0000ffffh ?
                vpand   ymm10, ymm10, ymm6              ; FFNum &= _cy-1 -- the entire carry report, one vpand per half
                vmovdqa [rsp+7F0h+var_110], ymm10       ; (spilled; reloaded at the very bottom of the loop)
                vpcmpeqd ymm6, ymm14, ymm8              ; /
                vpsubd  ymm8, ymm7, ymm6                ; n = (range<sTOP) + (range<gTOP), in 0..2
                vpslld  ymm6, ymm8, 3                   ; sh = n*8
                vpsrld  ymm10, ymm5, 10h                ; lowl>>16          \ the RC_SHIFT_SAT=0 form of the high-word
                vmovdqa ymm9, [rsp+7F0h+var_230]        ; 16                 | extract: _hi = (lowl>>16)>>(16-sh).  Two
                vpsubd  ymm13, ymm9, ymm6               ; 16-sh              | 32-bit shifts, so sh==0 needs no guard --
                vmovdqa ymm12, [rsp+7F0h+var_710]       ; tmpptr[0..7]       | that is what dropped the vpcmpeqd+vpandn
                vpaddd  ymm7, ymm12, ymm11              ; stad[0..7] = tmpptr-2
                vpsrlvd ymm13, ymm10, ymm13             ; _hi               /
                vpsllvd ymm11, ymm15, ymm6              ; lowh << sh
                vpsllvd ymm5, ymm5, ymm6                ; L: lowl <<= sh
                vmovdqa [rsp+7F0h+var_130], ymm5        ; (spill; reloaded near the bottom)
                vpsllvd ymm14, ymm14, ymm6              ; L: range <<= sh          <-- end of ShiftLow
                vpsrld  ymm5, ymm14, 0Fh                ; L: range>>15             <-- start of the coding step
                vmovdqa [rsp+7F0h+var_150], ymm5        ; spilled, and reloaded below as vpmulld's operand: improvement 2
                vpsubd  ymm12, ymm12, ymm8              ; tmpptr -= n.  n==0 for ~92% of lanes, so the staged store
                vmovdqa [rsp+7F0h+var_710], ymm12       ; above is dead -- what RC_SCATTER_SKIP tried to exploit
                vpaddd  ymm5, ymm2, [rsp+7F0h+var_1D0]  ; H: lowl += rpre  (rpre_H came off the top-of-loop spill)
                vpmaxud ymm8, ymm5, ymm2                ; \ carry out of lowl
                vpcmpeqd ymm8, ymm8, ymm5               ; /
                vpaddd  ymm8, ymm8, ymm4                ; \ lowh += _cf
                vpcmpeqd ymm2, ymm2, ymm2
                vpsubd  ymm8, ymm8, ymm2                ; /
                vpor    ymm2, ymm11, ymm13              ; L: lowh = (lowh<<sh) | _hi
                vmovdqa [rsp+7F0h+var_1D0], ymm2        ; parked in the slot rpre_H just vacated
                vpmaxud ymm11, ymm8, ymm4               ; \ H: carry out of lowh -> _cy-1 mask
                vpcmpeqd ymm11, ymm8, ymm11             ; /
                vmovdqa ymm10, [rsp+7F0h+var_250]       ; tmpptr[8..15]
                vextracti128 xmm12, ymm10, 1            ; \ lane 15's address, pulled out by hand -- 2 p5 uops for
                vpextrd r8d, xmm12, 3                   ; /  what vpaddd+spill 4 lines down already has.  Improvement 3
                vmovdqa ymm6, [rsp+7F0h+var_690]        ; \ FFNum[8..15] never gets a register: read and written
                vpand   ymm6, ymm11, ymm6               ;  | every single group.  Improvement 1
                vmovdqa [rsp+7F0h+var_690], ymm6        ; /
                vpaddd  ymm13, ymm10, [rsp+7F0h+var_170]  ; stad[8..15] = tmpptr-2
                vpminud ymm11, ymm1, ymm0               ; \ H: n = (range<sTOP) + (range<gTOP)
                vpcmpeqd ymm11, ymm11, ymm1
                vpsrld  ymm11, ymm11, 1Fh               ;  |
                vmovdqa [rsp+7F0h+var_330], ymm7        ; stad[0..7]  <-- STAGING STORE
                vpminud ymm12, ymm1, ymm3
                vpcmpeqd ymm12, ymm12, ymm1
                vpsubd  ymm11, ymm11, ymm12             ; /
                vpsubd  ymm10, ymm10, ymm11             ; H: tmpptr -= n
                vmovdqa [rsp+7F0h+var_250], ymm10
                vpslld  ymm10, ymm11, 3                 ; H: sh = n*8
                vmovdqa xmm11, xmmword ptr [r14+this*2+102000h]  ; pbit[i..i+7]: 8 packed (bit<<15)+p words, 16 bytes
                vpsrld  ymm12, ymm5, 10h                ; \ H: _hi = (lowl>>16)>>(16-sh)
                vpsubd  ymm6, ymm9, ymm10
                vpsrlvd ymm6, ymm12, ymm6               ; /
                vpsllvd ymm12, ymm8, ymm10              ; lowh << sh
                vpor    ymm4, ymm12, ymm6               ; H: lowh = (lowh<<sh) | _hi
                vpbroadcastd xmm0, cs:__real@7fff7fff   ; 0x7fff mask, rematerialised every group
                vpand   xmm6, xmm11, xmm0               ; L: p = b & 0x7FFF, still packed words
                vmovdqa [rsp+7F0h+var_750], ymm8        ; stcl[8..15] = lowh  <-- STAGING STORE
                vmovdqa [rsp+7F0h+var_310], ymm13       ; stad[8..15]         <-- STAGING STORE
                vpsllvd ymm2, ymm5, ymm10               ; H: lowl <<= sh
                vpmovzxwd ymm6, xmm6                    ; p -> dwords  (p5)
                vpsllvd ymm5, ymm1, ymm10               ; H: range <<= sh
                vmovdqa xmm1, xmmword ptr [r14+this*2+102010h]  ; pbit[i+8..i+15]
                vpextrd r9d, xmm7, 1                    ; the commit's addresses start coming out of ymm7 = stad_L
                vpmulld ymm6, ymm6, [rsp+7F0h+var_150]  ; L: rnew = (range>>15)*p.  2 uops, 10 cyc, operand reloaded
                vpand   xmm10, xmm1, xmm0               ; H: p = b & 0x7FFF
                vpextrd r10d, xmm7, 2
                vpextrd r11d, xmm7, 3
                vpmovzxwd ymm10, xmm10                  ; (p5)
                vmovq   rsi, xmm7                       ; stad[0] (packed with stad[1])
                vextracti128 xmm7, ymm7, 1              ; (p5)
                vmovd   edi, xmm7                       ; stad[4]
                vpsrld  ymm7, ymm5, 0Fh                 ; H: range>>15 -- kept in a REGISTER here, unlike the L half
                vpmulld ymm10, ymm7, ymm10              ; H: rnew = (range>>15)*p
                vpsubd  ymm7, ymm14, ymm6               ; L: range - rnew
                vpxor   xmm0, xmm0, xmm0
                vpcmpgtw xmm11, xmm0, xmm11             ; \ the bit mask straight off the word's SIGN: (bit<<15)+p is
                vpmovsxwd ymm11, xmm11                  ; /  negative as a word iff bit==1.  2 p5 uops -- see impr. 5
                vblendvps ymm14, ymm6, ymm7, ymm11      ; L: range = bit ? range-rnew : rnew
                vpand   ymm7, ymm11, ymm6               ; L: rpre  = bit ? rnew : 0    (the deferred low update)
                vpsubd  ymm5, ymm5, ymm10               ; H: range - rnew
                vpcmpgtw xmm1, xmm0, xmm1               ; \ H: the same sign trick
                vmovdqa ymm0, [rsp+7F0h+var_130]        ; L: reload lowl
                vpmovsxwd ymm6, xmm1                    ; /
                vblendvps ymm1, ymm10, ymm5, ymm6       ; H: range = bit ? range-rnew : rnew
                vpand   ymm6, ymm10, ymm6               ; H: rpre  = bit ? rnew : 0
                vmovaps ymm10, [rsp+7F0h+var_110]       ; L: reload FFNum
;  --- RC_FLUSHALL: 16 x `(uint&)tmpbase[stad[k]] = stcl[k]`.
;      tmpbase folds into the addressing mode as the +11000h displacement
;      off r14, so each store is one instruction with no base setup.
;
;      Clang split the sixteen three ways, and the split is the interesting
;      part: 9 lanes take their address out of a vector register (vpextrd /
;      vmovq / vextracti128 -- p5 uops) and 7 reload it from the staged array
;      it has just written (2 loads + 1 store -- p23 and p4 uops).  That is
;      the balance it found between port 5 and the rename width; RC_SCATTER=0,
;      which extracts all sixteen, measures +8.43 cyc/grp worse.  The other
;      endpoint -- force all sixteen through memory -- is untried, and would
;      add ~9 loads to a loop that is already rename-bound, so expect a loss.
                mov     esi, esi                        ; zero-extend stad[0] out of the packed qword
                vmovd   dword ptr [r14+rsi+11000h], xmm15  ; lane 0
                vpextrd dword ptr [r14+r9+11000h], xmm15, 1  ; lane 1
                vpextrd dword ptr [r14+r10+11000h], xmm15, 2  ; lane 2
                vpextrd dword ptr [r14+r11+11000h], xmm15, 3  ; lane 3
                vextracti128 xmm5, ymm15, 1             ; (p5)
                vmovd   dword ptr [r14+rdi+11000h], xmm5  ; lane 4
                mov     r9d, dword ptr [rsp+7F0h+var_770+14h]  ; \ lane 5: value AND address both reloaded from the staged
                mov     r10d, dword ptr [rsp+7F0h+var_330+14h]
                mov     [r14+r10+11000h], r9d           ; /  arrays.  3 uops, none of them p5
                mov     r9d, dword ptr [rsp+7F0h+var_770+18h]  ; \ lane 6
                mov     r10d, dword ptr [rsp+7F0h+var_330+18h]
                mov     [r14+r10+11000h], r9d           ; /
                mov     r9d, dword ptr [rsp+7F0h+var_770+1Ch]  ; \ lane 7
                mov     r10d, dword ptr [rsp+7F0h+var_330+1Ch]
                mov     [r14+r10+11000h], r9d           ; /
                vmovq   r9, xmm13                       ; \ stad[8] out of the packed qword
                mov     r9d, r9d                        ; /
                vpextrd r10d, xmm13, 1
                vmovd   dword ptr [r14+r9+11000h], xmm8 ; lane 8
                vpextrd dword ptr [r14+r10+11000h], xmm8, 1  ; lane 9
                vmovdqa ymm8, [rsp+7F0h+var_1D0]        ; L: reload lowh for the next pass
                vpextrd r9d, xmm13, 2                   ; lane 10's address from the vector ...
                add     r8d, 0FFFFFFFEh                 ; ... and lane 15's, finished off from the vextracti128 above
                mov     r10d, dword ptr [rsp+7F0h+var_750+8]  ; \ lane 10: address from a register, value from memory
                mov     [r14+r9+11000h], r10d           ; /
                mov     r9d, dword ptr [rsp+7F0h+var_750+0Ch]  ; \ lane 11
                mov     r10d, dword ptr [rsp+7F0h+var_310+0Ch]
                mov     [r14+r10+11000h], r9d           ; /
                mov     r9d, dword ptr [rsp+7F0h+var_750+10h]  ; \ lane 12
                mov     r10d, dword ptr [rsp+7F0h+var_310+10h]
                mov     [r14+r10+11000h], r9d           ; /
                mov     r9d, dword ptr [rsp+7F0h+var_750+14h]  ; \ lane 13
                mov     r10d, dword ptr [rsp+7F0h+var_310+14h]
                mov     [r14+r10+11000h], r9d           ; /
                mov     r9d, dword ptr [rsp+7F0h+var_750+18h]  ; \ lane 14
                mov     r10d, dword ptr [rsp+7F0h+var_310+18h]
                mov     [r14+r10+11000h], r9d           ; /
                mov     r9d, dword ptr [rsp+7F0h+var_750+1Ch]  ; \ lane 15
                mov     [r14+r8+11000h], r9d            ; /
                add     this, 10h                       ; i += RCNUM.  add/cmp/jb is 2 fused uops; a negative index
                cmp     this, rdx                       ; counting up to zero would make it add/jne = 1.  Improvement 6
                jb      loc_1400C15B0
;  --- loop exit: write the working registers back to the canonical slots ---
                vmovdqa [rsp+7F0h+var_370], ymm0
                vmovdqa [rsp+7F0h+var_3B0], ymm7
                vmovdqa [rsp+7F0h+var_2F0], ymm8
                vmovaps [rsp+7F0h+var_6F0], ymm10
                vmovaps ymm0, [rsp+7F0h+var_710]
                vmovaps [rsp+7F0h+var_2B0], ymm0
                vmovaps [rsp+7F0h+var_3F0], ymm14
                vmovdqa [rsp+7F0h+var_350], ymm2
                vmovdqa [rsp+7F0h+var_390], ymm6
                vmovdqa [rsp+7F0h+var_2D0], ymm4
                vmovaps ymm0, [rsp+7F0h+var_690]
                vmovaps [rsp+7F0h+var_6D0], ymm0
                vmovdqa ymm0, [rsp+7F0h+var_250]
                vmovdqa [rsp+7F0h+var_290], ymm0
                vmovaps [rsp+7F0h+var_3D0], ymm1
                vpxor   xmm9, xmm9, xmm9
                vxorps  xmm10, xmm10, xmm10
                vmovdqa ymm11, [rsp+7F0h+var_1F0]
                vmovdqa ymm12, [rsp+7F0h+var_210]
                vpcmpeqd ymm13, ymm13, ymm13
                vmovdqa ymm14, [rsp+7F0h+var_230]
                vmovq   xmm15, cs:__xmm@00000000000000000001020304050607
                cmp     ecx, eax
                jb      loc_1400C1DBF                   ; 8 <= bits left -> the vectorised tail at +19FFh
                jmp     loc_1400C2030                   ; nothing left -> rc_Quit
; ---------------------------------------------------------------------------
                align 10h

;=============================================================================
;  +1670h .. +19F4h   THE -C SCALAR CODER'S THREE LOOPS.  COLD -- reached
;  only with -C.  All three are the same shape: rc_Process inlined with the
;  branchy ShiftLow (`jz` past the common n==0 case, then one or two
;  byte emits), over rcC's 64-byte lane records.
;
;    +1670h loc_1400C1A30  the 16-bit length header, `add rcx,40h` per lane
;    +17A0h loc_1400C1B60  the main group loop: outer i+=16 with `add r8,20h`
;                          over pbit, inner j=0..15 with `add r9,40h`
;    +18D0h loc_1400C1C90  the tail, one bit at a time, lane = (i%16)*64
;
;  Nothing to improve here on purpose: this is the reference implementation,
;  and its only job is to be obviously the same arithmetic as rc.inc.
;=============================================================================
loc_1400C1A30:                          ; CODE XREF: Model_0___do_process+E07^j
                sub     ecx, eax
                mov     [r14+202074h], ecx
                mov     [r14+202068h], eax
                mov     eax, 0Fh
                xor     ecx, ecx
                jmp     short loc_1400C1A9F
; ---------------------------------------------------------------------------
                align 10h

loc_1400C1A50:                          ; CODE XREF: Model_0___do_process+1763vj
                                        ; Model_0___do_process+178Dvj
                mov     r8d, [rsi+this-4]

loc_1400C1A55:                          ; CODE XREF: Model_0___do_process+1716vj
                shl     edx, 3
                shlx    edx, r8d, edx
                mov     r8d, edx
                shr     r8d, 1
                and     r8d, 7FFFC000h
                sub     edx, r8d
                mov     r9, [rsp+7F0h+var_7A0]
                bt      r9d, eax
                cmovnb  edx, r8d
                mov     [rsi+this-4], edx
                mov     edx, 0
                cmovnb  r8d, edx
                mov     [rsi+this-10h], r8d
                add     this, 40h ; '@'
                dec     eax
                cmp     this, 400h
                jz      loc_1400C1B60

loc_1400C1A9F:                          ; CODE XREF: Model_0___do_process+1687^j
                mov     r8d, [rsi+this-4]
                xor     edx, edx
                cmp     r8d, 1000000h
                setb    dl
                mov     r9d, [rsi+this-14h]
                mov     r10d, [rsi+this-10h]
                add     [rsi+this-18h], r10d
                adc     r9d, 0
                adc     dword ptr [rsi+this], 0
                cmp     r8d, 10000h
                adc     edx, 0
                mov     [rsi+this-14h], r9d
                jz      loc_1400C1A55
                cmp     r8d, 10000h
                setb    r10b
                cmp     r8d, 1000000h
                setb    r11b
                shr     r9d, 18h
                mov     r8, [rsi+this-30h]
                lea     rsi, [r8-1]
                mov     rdi, [rsp+7F0h+var_798]
                mov     [rdi+this-30h], rsi
                mov     rsi, [rsp+7F0h+var_798]
                mov     [r8], r9b
                mov     r8d, [rsi+this-15h]
                mov     [rsi+this-14h], r8d
                shl     dword ptr [rsi+this-18h], 8
                xor     r11b, r10b
                jnz     loc_1400C1A50
                shr     r8d, 18h
                mov     r9, [rsi+this-30h]
                lea     r10, [r9-1]
                mov     [rsi+this-30h], r10
                mov     [r9], r8b
                mov     r8d, [rsi+this-15h]
                mov     [rsi+this-14h], r8d
                shl     dword ptr [rsi+this-18h], 8
                jmp     loc_1400C1A50
; ---------------------------------------------------------------------------
                align 20h

loc_1400C1B60:                          ; CODE XREF: Model_0___do_process+16D9^j
                mov     rax, [rsp+7F0h+var_7A0]
                lea     eax, ds:0[rax*8]
                mov     ecx, eax
                and     ecx, 7FFF0h
                jz      loc_1400C250A

loc_1400C1B7A:                          ; CODE XREF: Model_0___do_process+E29^j
                mov     r8, [rsp+7F0h+var_790]
                mov     edx, ecx
                xor     ecx, ecx
                jmp     short loc_1400C1BA6
; ---------------------------------------------------------------------------
                align 10h

loc_1400C1B90:                          ; CODE XREF: Model_0___do_process+1832vj
                add     this, 10h
                add     r8, 20h ; ' '
                cmp     this, rdx
                mov     rsi, [rsp+7F0h+var_798]
                jnb     loc_1400C1C90

loc_1400C1BA6:                          ; CODE XREF: Model_0___do_process+17C3^j
                mov     r9, rsi
                xor     r10d, r10d
                jmp     short loc_1400C1BF4
; ---------------------------------------------------------------------------
                align 10h

loc_1400C1BB0:                          ; CODE XREF: Model_0___do_process+18A0vj
                                        ; Model_0___do_process+18C5vj
                mov     ebx, [r9-4]

loc_1400C1BB4:                          ; CODE XREF: Model_0___do_process+1869vj
                mov     edi, r11d
                and     edi, 7FFFh
                shr     r11d, 0Fh
                shl     esi, 3
                shlx    esi, ebx, esi
                mov     ebx, esi
                shr     ebx, 0Fh
                imul    ebx, edi
                sub     esi, ebx
                test    r11d, r11d
                cmovz   esi, ebx
                mov     [r9-4], esi
                neg     r11d
                and     r11d, ebx
                mov     [r9-10h], r11d
                inc     r10
                add     r9, 40h ; '@'
                cmp     r10, 10h
                jz      short loc_1400C1B90

loc_1400C1BF4:                          ; CODE XREF: Model_0___do_process+17EC^j
                movzx   r11d, word ptr [r8+r10*2]
                mov     ebx, [r9-4]
                xor     esi, esi
                cmp     ebx, 1000000h
                setb    sil
                mov     edi, [r9-14h]
                mov     r14d, [r9-10h]
                add     [r9-18h], r14d
                adc     edi, 0
                adc     dword ptr [r9], 0
                cmp     ebx, 10000h
                adc     esi, 0
                mov     [r9-14h], edi
                jz      short loc_1400C1BB4
                cmp     ebx, 10000h
                setb    r14b
                cmp     ebx, 1000000h
                setb    bl
                shr     edi, 18h
                mov     r15, [r9-30h]
                lea     r12, [r15-1]
                mov     [r9-30h], r12
                mov     [r15], dil
                mov     edi, [r9-15h]
                mov     [r9-14h], edi
                shl     dword ptr [r9-18h], 8
                xor     bl, r14b
                jnz     loc_1400C1BB0
                shr     edi, 18h
                mov     rbx, [r9-30h]
                lea     r14, [rbx-1]
                mov     [r9-30h], r14
                mov     [rbx], dil
                mov     edi, [r9-15h]
                mov     [r9-14h], edi
                shl     dword ptr [r9-18h], 8
                jmp     loc_1400C1BB0
; ---------------------------------------------------------------------------
                align 10h

loc_1400C1C90:                          ; CODE XREF: Model_0___do_process+17E0^j
                mov     r14, qword ptr [rsp+7F0h+blksize]
                cmp     ecx, eax
                jnb     loc_1400C2520

loc_1400C1C9D:                          ; CODE XREF: Model_0___do_process+214Evj
                mov     ecx, ecx
                mov     eax, eax
                jmp     short loc_1400C1CF7
; ---------------------------------------------------------------------------
                align 10h

loc_1400C1CB0:                          ; CODE XREF: Model_0___do_process+19CAvj
                                        ; Model_0___do_process+19F0vj
                mov     r10d, [r8+34h]

loc_1400C1CB4:                          ; CODE XREF: Model_0___do_process+198Cvj
                mov     r11d, edx
                and     r11d, 7FFFh
                shr     edx, 0Fh
                shl     r9d, 3
                shlx    r9d, r10d, r9d
                mov     r10d, r9d
                shr     r10d, 0Fh
                imul    r10d, r11d
                sub     r9d, r10d
                test    edx, edx
                cmovz   r9d, r10d
                mov     [r8+34h], r9d
                neg     edx
                and     edx, r10d
                mov     [r8+28h], edx
                inc     this
                cmp     rax, this
                jz      loc_1400C2520

loc_1400C1CF7:                          ; CODE XREF: Model_0___do_process+18E1^j
                mov     rdx, [rsp+7F0h+var_790]
                movzx   edx, word ptr [rdx+this*2]
                mov     esi, ecx
                and     esi, 0Fh
                shl     esi, 6
                mov     rdi, [rsp+7F0h+var_780]
                mov     r10d, [rdi+rsi+34h]
                xor     r9d, r9d
                cmp     r10d, 1000000h
                setb    r9b
                mov     r11d, [rdi+rsi+24h]
                mov     r8d, [rdi+rsi+28h]
                add     [rdi+rsi+20h], r8d
                adc     r11d, 0
                adc     dword ptr [rdi+rsi+38h], 0
                lea     r8, [rdi+rsi]
                cmp     r10d, 10000h
                adc     r9d, 0
                mov     [rdi+rsi+24h], r11d
                jz      loc_1400C1CB4
                cmp     r10d, 10000h
                setb    bl
                cmp     r10d, 1000000h
                setb    sil
                shr     r11d, 18h
                mov     r10, [r8+8]
                lea     rdi, [r10-1]
                mov     [r8+8], rdi
                mov     [r10], r11b
                mov     r10d, [r8+23h]
                mov     [r8+24h], r10d
                shl     dword ptr [r8+20h], 8
                xor     sil, bl
                jnz     loc_1400C1CB0
                shr     r10d, 18h
                mov     r11, [r8+8]
                lea     rsi, [r11-1]
                mov     [r8+8], rsi
                mov     [r11], r10b
                mov     r10d, [r8+23h]
                mov     [r8+24h], r10d
                shl     dword ptr [r8+20h], 8
                jmp     loc_1400C1CB0
; ---------------------------------------------------------------------------

;=============================================================================
;  +19F5h  THE VECTOR TAIL --  for( ; i<blksize0; i++ )
;
;  Consecutive bits, so lane = i%RCNUM walks 0,1,2,...  Clang vectorised it
;  8 bits at a time by indexing the lane arrays at the UNALIGNED byte offset
;  (i%16)*4 -- a rotating 8-lane window over the 16-lane arrays -- which is
;  why the next dozen instructions are runtime checks that the window cannot
;  wrap.  Then a scalar remainder at +2340h.
;
;  blksize0 = blksize*8 and blksize1 = blksize0 & ~15, so this only runs at
;  all when blksize is ODD.  For a full 65536-byte block it never runs: cold.
;=============================================================================
loc_1400C1DB5:                          ; CODE XREF: Model_0___do_process+118D^j
                xor     ecx, ecx
                cmp     ecx, eax
                jnb     loc_1400C2030

loc_1400C1DBF:                          ; CODE XREF: Model_0___do_process+1656^j
                mov     edx, ecx
                mov     eax, eax
                mov     r8, rax
                sub     r8, rdx
                cmp     r8, 7
                jbe     loc_1400C2799                   ; fewer than 8 bits left -> straight to the scalar remainder
                mov     r9, rdx
                not     r9
                add     r9, rax
                and     cl, 0Fh
                mov     r10d, r9d
                and     r10b, 0Fh
                add     r10b, cl                        ; \ (i%16) + (count-1)%16 must not carry into bit 4 ...
                test    r10b, 10h
                jnz     loc_1400C2799                   ; /
                cmp     r9, 0Fh
                ja      loc_1400C2799                   ; ... and the whole run must fit one 16-lane window
                mov     qword ptr [rsp+7F0h+var_710], r8
                mov     r10, r8
                and     r10, 0FFFFFFFFFFFFFFF8h         ; trip count rounded down to a multiple of 8
                mov     r9, rdx
                mov     qword ptr [rsp+7F0h+var_690], r10
                mov     r8, [rsp+7F0h+var_790]
                nop     dword ptr [rax+00h]

;  --- the 8-wide tail body.  Same arithmetic as the hot loop, one half of
;      it, but every lane-array access is a vmovdqu at [rsp + (i%16)*4].
;      The commit is a full 8-lane extract-and-store, and r13 (tmpbase) is
;      reloaded from [C] before EVERY ONE of the eight stores because the
;      extraction clobbers it.  Nine loads of a loop-invariant value: ugly,
;      and completely irrelevant -- at most 8 bits of the last block of a
;      file pass through here.
loc_1400C1E20:                          ; CODE XREF: Model_0___do_process+1C4Fvj
                vpmovzxwd ymm0, xmmword ptr [r8+r9*2]   ; pbit[i..i+7], zero-extended straight to dwords
                vpbroadcastd ymm1, cs:__real@00007fff
                vpand   ymm2, ymm0, ymm1                ; p = b & 0x7FFF
                vpsrld  ymm1, ymm0, 0Fh                 ; bit = b >> 15  (a dword compare here, not the word sign trick)
                mov     r11d, r9d
                and     r11d, 0Fh
                shl     r11d, 2                         ; (i%16)*4 -- the rotating window's byte offset
                vmovdqu ymm3, [rsp+r11+7F0h+var_3F0]    ; range[]
                vpminud ymm0, ymm3, ymm11
                vpcmpeqd ymm0, ymm3, ymm0
                vpsrld  ymm4, ymm0, 1Fh
                vpminud ymm0, ymm3, ymm12
                vpcmpeqd ymm5, ymm3, ymm0
                vmovdqu ymm0, [rsp+r11+7F0h+var_370]    ; lowl[]
                vpaddd  ymm6, ymm0, [rsp+r11+7F0h+var_3B0]  ; lowl += rpre
                vpmaxud ymm0, ymm6, ymm0
                vpcmpeqd ymm0, ymm6, ymm0
                vmovdqu ymm7, [rsp+r11+7F0h+var_2F0]
                vpaddd  ymm0, ymm7, ymm0
                vpsubd  ymm0, ymm0, ymm13
                vpmaxud ymm7, ymm0, ymm7
                vpcmpeqd ymm7, ymm0, ymm7
                vpxor   ymm7, ymm13, ymm7
                vpmaskmovd [rsp+r11+7F0h+var_6F0], ymm7, ymm9  ; FFNum &= _cy-1, as a masked store of zero
                vmovdqu [rsp+r11+7F0h+var_770], ymm0    ; stcl[] = lowh
                vpsubd  ymm4, ymm4, ymm5
                vmovdqu ymm5, [rsp+r11+7F0h+var_2B0]
                vpbroadcastd ymm7, cs:__real@fffffffe
                vpaddd  ymm7, ymm5, ymm7
                vmovdqu [rsp+r11+7F0h+var_330], ymm7    ; stad[] = tmpptr-2
                vpslld  ymm8, ymm4, 3
                vpsubd  ymm4, ymm5, ymm4
                vmovdqu [rsp+r11+7F0h+var_2B0], ymm4    ; tmpptr -= n
                vpsrld  ymm4, ymm6, 10h
                vpsubd  ymm5, ymm14, ymm8
                vpsrlvd ymm4, ymm4, ymm5
                vpsllvd ymm5, ymm0, ymm8
                vpor    ymm4, ymm5, ymm4
                vmovdqu [rsp+r11+7F0h+var_2F0], ymm4
                vpsllvd ymm4, ymm6, ymm8
                vmovdqu [rsp+r11+7F0h+var_370], ymm4
                vpsllvd ymm3, ymm3, ymm8
                vpsrld  ymm4, ymm3, 0Fh
                vpmulld ymm2, ymm4, ymm2                ; rnew = (range>>15)*p
                vpsubd  ymm3, ymm3, ymm2
                vpcmpeqd ymm4, ymm9, ymm1
                vblendvps ymm3, ymm3, ymm2, ymm4
                vmovups [rsp+r11+7F0h+var_3F0], ymm3
                vpsubd  ymm1, ymm9, ymm1
                vpand   ymm1, ymm2, ymm1
                vmovdqu [rsp+r11+7F0h+var_3B0], ymm1
                vextracti128 xmm1, ymm7, 1              ; \ eight 32-bit addresses widened to qwords ...
                vpmovzxdq ymm1, xmm1
                vpmovzxdq ymm2, xmm7
                vmovd   r11d, xmm2
                vpextrq rsi, xmm2, 1
                vextracti128 xmm2, ymm2, 1
                vmovd   edi, xmm2
                vpextrq rbx, xmm2, 1
                vmovd   r14d, xmm1
                vpextrq r15, xmm1, 1
                vextracti128 xmm1, ymm1, 1
                vmovd   r12d, xmm1
                vpextrq r13, xmm1, 1
                mov     this, [rsp+7F0h+C]              ; /  ... then eight stores, each one RELOADING tmpbase because
                vmovd   dword ptr [this+r11], xmm0
                mov     this, [rsp+7F0h+C]              ;    the extraction above clobbered r13.  Nine loads of a
                vpextrd dword ptr [this+rsi], xmm0, 1
                mov     this, [rsp+7F0h+C]              ;    loop-invariant.  Irrelevant: this runs at most once
                vpextrd dword ptr [this+rdi], xmm0, 2
                mov     this, [rsp+7F0h+C]
                vpextrd dword ptr [this+rbx], xmm0, 3
                vextracti128 xmm0, ymm0, 1
                mov     this, [rsp+7F0h+C]
                vmovd   dword ptr [this+r14], xmm0
                mov     this, [rsp+7F0h+C]
                vpextrd dword ptr [this+r15], xmm0, 1
                mov     this, [rsp+7F0h+C]
                vpextrd dword ptr [this+r12], xmm0, 2
                mov     this, [rsp+7F0h+C]
                vpextrd dword ptr [this+r13], xmm0, 3
                mov     r13, [rsp+7F0h+C]
                add     r9, 8
                add     r10, 0FFFFFFFFFFFFFFF8h
                jnz     loc_1400C1E20
                mov     this, qword ptr [rsp+7F0h+var_690]
                cmp     qword ptr [rsp+7F0h+var_710], this
                jnz     loc_1400C26F3
                nop     dword ptr [rax+rax+00h]

;=============================================================================
;  +1C70h  rc_Quit x RCNUM, for the vector path.  Once per block.
;
;  Per lane: one last ShiftLow-shaped renorm, then the CARRYLESS MINIMAL
;  FLUSH -- emit only the low bytes the decoder cannot reconstruct as 0xFF.
;  The `or 0FFh / or 0FFFFh / ... / or 0FFFFFFFFFFFFFFh` ladder with setb and
;  cmovnb is that count: how many of low's top bytes can be dropped because
;  saturating them to all-ones still lands below the interval's top.
;
;  Scalarised (rax counts lanes 0..15) because each lane emits a different
;  NUMBER of bytes.  ~1600 instructions per 65536-byte block, i.e. 0.024 per
;  input byte.  Not worth touching.
;=============================================================================
loc_1400C2030:                          ; CODE XREF: Model_0___do_process+165C^j
                                        ; Model_0___do_process+19F9^j ...
                xor     eax, eax
                vmovdqa ymm2, cs:__ymm@80808018808080118080801c808080158080800a808080038080800e80808007
                jmp     short loc_1400C205F
; ---------------------------------------------------------------------------
                align 20h

loc_1400C2040:                          ; CODE XREF: Model_0___do_process+1D68vj
                inc     r10d

loc_1400C2043:                          ; CODE XREF: Model_0___do_process+1EE3vj
                mov     ecx, r10d

loc_1400C2046:                          ; CODE XREF: Model_0___do_process+1F0Avj
                                        ; Model_0___do_process+1FB3vj
                mov     dword ptr [rsp+rax*4+7F0h+var_2B0], ecx  ; tmpptr[] for this lane
                inc     rax
                cmp     rax, 10h
                mov     r13, [rsp+7F0h+C]
                jz      loc_1400C2380

loc_1400C205F:                          ; CODE XREF: Model_0___do_process+1C7A^j
                mov     ecx, dword ptr [rsp+rax*4+7F0h+var_3F0]  ; range[] for lane rax
                xor     r11d, r11d
                cmp     ecx, 1000000h                   ; \ n = (range<sTOP) + (range<gTOP), scalar this time
                setb    r11b
                cmp     ecx, 10000h
                adc     r11d, 0                         ; /
                lea     edx, ds:0[r11*8]                ; sh = n*8
                mov     r9d, dword ptr [rsp+rax*4+7F0h+var_3B0]  ; \ low += rpre, 64-bit
                xor     r10d, r10d
                add     r9d, dword ptr [rsp+rax*4+7F0h+var_370]
                mov     r8d, dword ptr [rsp+rax*4+7F0h+var_2F0]
                adc     r8d, 0                          ; /
                mov     dword ptr [rsp+rax*4+7F0h+var_3B0], 0  ; rpre = 0
                jb      short loc_1400C20B9
                mov     r10d, dword ptr [rsp+rax*4+7F0h+var_6F0]  ; FFNum: kept if that add did not carry, zeroed if it did

loc_1400C20B9:                          ; CODE XREF: Model_0___do_process+1CEF^j
                mov     dword ptr [rsp+rax*4+7F0h+var_6F0], r10d
                mov     dword ptr [rsp+rax*4+7F0h+var_770], r8d  ; stcl = lowh
                mov     r10d, dword ptr [rsp+rax*4+7F0h+var_2B0]
                lea     esi, [r10-2]
                mov     dword ptr [rsp+rax*4+7F0h+var_330], esi  ; stad = tmpptr-2
                sub     r10d, r11d                      ; tmpptr -= n
                mov     r11d, r9d
                shr     r11d, 10h                       ; \ _hi = (lowl>>16)>>(16-sh) ...
                mov     dil, 10h
                sub     dil, dl
                shrx    r11d, r11d, edi
                shlx    edi, r8d, edx
                or      edi, r11d                       ; /  ... lowh = (lowh<<sh) | _hi
                mov     dword ptr [rsp+rax*4+7F0h+var_2F0], edi
                shlx    r9d, r9d, edx
                mov     dword ptr [rsp+rax*4+7F0h+var_370], r9d
                shlx    ecx, ecx, edx                   ; range <<= sh
                mov     dword ptr [rsp+rax*4+7F0h+var_3F0], ecx
                mov     [r13+rsi+0], r8d                ; the last staged store, committed on the spot
                shl     rdi, 20h                        ; \ low back as one qword ...
                or      r9, rdi
                add     this, r9                        ; /  ... and the running interval top
                jb      loc_1400C2040
                mov     rdx, r9                         ; \ the minimal flush: 8 nested `is low|00..FFh still below
                or      rdx, 0FFh                       ;  |  top` tests, each setb + cmovnb.  rbx ends up as the
                cmp     rdx, this                       ; /  number of bytes that actually have to be emitted
                mov     esi, 8
                sbb     rsi, 0
                cmp     rdx, this
                cmovnb  rdx, r9
                mov     r9, rdx
                or      r9, 0FFFFh
                xor     r11d, r11d
                cmp     r9, this
                setb    r11b
                cmovnb  r9, rdx
                mov     rbx, rsi
                sub     rbx, r11
                mov     rdx, r9
                or      rdx, 0FFFFFFh
                xor     edi, edi
                cmp     rdx, this
                setb    dil
                cmovnb  rdx, r9
                sub     rbx, rdi
                mov     r9, rdx
                mov     r8d, 0FFFFFFFFh
                or      r9, r8
                xor     r15d, r15d
                cmp     r9, this
                setb    r15b
                cmovnb  r9, rdx
                sub     rbx, r15
                mov     rdx, r9
                mov     r8, 0FFFFFFFFFFh
                or      rdx, r8
                xor     r13d, r13d
                cmp     rdx, this
                setb    r13b
                cmovnb  rdx, r9
                sub     rbx, r13
                mov     r9, rdx
                mov     r8, 0FFFFFFFFFFFFh
                or      r9, r8
                xor     r14d, r14d
                cmp     r9, this
                setb    r14b
                cmovnb  r9, rdx
                sub     rbx, r14
                mov     rdx, r9
                mov     r8, 0FFFFFFFFFFFFFFh
                or      rdx, r8
                xor     r12d, r12d
                cmp     rdx, this
                setb    r12b
                cmovnb  rdx, r9
                sub     rbx, r12
                lea     ecx, [r10+1]
                cmp     rbx, 8
                jnb     short loc_1400C2260
                mov     qword ptr [rsp+7F0h+var_250], rdi
                mov     qword ptr [rsp+7F0h+var_690], rsi
                mov     qword ptr [rsp+7F0h+var_710], r11
                mov     r10, rbx
                and     r10, 3
                jz      short loc_1400C22A8
                mov     edi, 38h ; '8'
                xor     r9d, r9d
                mov     r11, [rsp+7F0h+C]
                nop     dword ptr [rax+rax+00h]

;  --- three ways to emit the remaining bytes: one at a time (below),
;      eight at once with a vpshufb byte-reverse (+1EAAh), or four at a
;      time (+1F10h).  The stream is written backwards, so the 8-byte
;      case is a bswap into one vmovq.
loc_1400C2240:                          ; CODE XREF: Model_0___do_process+1E98vj
                shrx    rsi, rdx, rdi
                mov     r8d, ecx
                dec     ecx
                mov     [r11+r8], sil
                inc     r9
                add     rdi, 0FFFFFFFFFFFFFFF8h
                cmp     r10, r9
                jnz     short loc_1400C2240
                jmp     short loc_1400C22AB
; ---------------------------------------------------------------------------
                align 20h

loc_1400C2260:                          ; CODE XREF: Model_0___do_process+1E4B^j
                cmp     ecx, 7
                jnb     short loc_1400C226A
                xor     r9d, r9d
                jmp     short loc_1400C22D0
; ---------------------------------------------------------------------------

loc_1400C226A:                          ; CODE XREF: Model_0___do_process+1EA3^j
                vmovq   xmm0, rdx
                vpbroadcastq ymm0, xmm0
                vpshufb ymm0, ymm0, ymm2
                vextracti128 xmm1, ymm0, 1
                vpackusdw xmm0, xmm0, xmm1
                vpshufd xmm0, xmm0, 0D8h
                vpackuswb xmm0, xmm0, xmm0
                vpshufb xmm0, xmm0, xmm15
                mov     ecx, ecx
                mov     rdx, [rsp+7F0h+C]
                vmovq   qword ptr [rdx+this-7], xmm0
                add     r10d, 0FFFFFFF9h
                jmp     loc_1400C2043
; ---------------------------------------------------------------------------

loc_1400C22A8:                          ; CODE XREF: Model_0___do_process+1E6C^j
                xor     r9d, r9d

loc_1400C22AB:                          ; CODE XREF: Model_0___do_process+1E9A^j
                dec     rbx
                cmp     rbx, 3
                mov     r11, qword ptr [rsp+7F0h+var_710]
                mov     rsi, qword ptr [rsp+7F0h+var_690]
                mov     rdi, qword ptr [rsp+7F0h+var_250]
                jb      loc_1400C2046

loc_1400C22D0:                          ; CODE XREF: Model_0___do_process+1EA8^j
                neg     r11
                neg     rdi
                neg     r15
                neg     r13
                neg     r14
                neg     r12
                mov     rbx, r9
                sub     rbx, rsi
                sub     rbx, rdi
                sub     rbx, r15
                sub     rbx, r13
                sub     rbx, r14
                sub     rbx, r12
                sub     rbx, r11
                shl     r9, 3
                mov     r10d, 20h ; ' '
                sub     r10, r9
                mov     r8d, ecx
                lea     r9d, [this-3]
                lea     r11d, [this-2]
                dec     ecx
                xor     esi, esi
                mov     r12, [rsp+7F0h+C]
                nop     dword ptr [rax+rax+00h]

loc_1400C2320:                          ; CODE XREF: Model_0___do_process+1FABvj
                lea     edi, [r8+rsi]
                lea     r14d, [r10+18h]
                shrx    r14, rdx, r14
                lea     r15d, [this+rsi]
                mov     [r12+rdi], r14b
                lea     edi, [r10+10h]
                shrx    rdi, rdx, rdi
                lea     r14d, [r11+rsi]
                mov     [r12+r15], dil
                lea     edi, [r10+8]
                shrx    rdi, rdx, rdi
                lea     r15d, [r9+rsi]
                mov     [r12+r14], dil
                shrx    rdi, rdx, r10
                mov     [r12+r15], dil
                add     rsi, 0FFFFFFFFFFFFFFFCh
                add     r10, 0FFFFFFFFFFFFFFE0h
                cmp     rbx, rsi
                jnz     short loc_1400C2320
                add     r8d, esi
                mov     ecx, r8d
                jmp     loc_1400C2046
; ---------------------------------------------------------------------------
                align 20h

;=============================================================================
;  +1FC0h  PER-LANE LENGTHS, OVERFLOW CHECK, CARRY FOLD, then rc_Write
;
;      len = ((j+1)*rowsize-1) - tmpptr[j];
;      len = len>RC_SKIP ? len-RC_SKIP : 0;   res.lens[j] = len;
;      ncarry &= FFNum[j];  novf += (len>tmpbufsize);
;
;  All of it vectorised.  Once per block.
;=============================================================================
loc_1400C2380:                          ; CODE XREF: Model_0___do_process+1C99^j
                vmovdqa ymm0, cs:__ymm@0007855f000694b30005a4070004b35b0003c2af0002d2030001e1570000f0ab  ; (j+1)*rowsize-1 for lanes 0..7
                vpsubd  ymm0, ymm0, [rsp+7F0h+var_2B0]  ; len = that - tmpptr[j]
                vpbroadcastd ymm3, cs:__real@00000004   ; RC_SKIP = 4
                vpmaxud ymm0, ymm0, ymm3                ; \ max(len,4)-4 IS the `len>RC_SKIP ? len-RC_SKIP : 0`
                vpsubd  ymm0, ymm0, ymm3                ; /
                mov     r14, qword ptr [rsp+7F0h+blksize]
                vmovdqa ymmword ptr [r14+202000h], ymm0 ; res.lens[0..7]
                vpbroadcastd ymm4, cs:__real@0000f068   ; tmpbufsize = F068h
                vpmaxud ymm1, ymm0, ymm4                ; \ len > tmpbufsize ?
                vpcmpeqd ymm0, ymm0, ymm1               ; /
                vextracti128 xmm1, ymm0, 1
                vpackssdw xmm5, xmm0, xmm1
                vmovdqa ymm0, [rsp+7F0h+var_6F0]
                vmovdqa xmm2, xmmword ptr [rsp+7F0h+var_6D0]
                vmovq   xmm1, qword ptr [rsp+7F0h+var_6D0+10h]
                vmovdqa ymm6, cs:__ymm@000f0abf000e1a13000d2967000c38bb000b480f000a5763000966b70008760b
                vpsubd  ymm6, ymm6, [rsp+7F0h+var_290]  ; the same for lanes 8..15
                mov     eax, dword ptr [rsp+7F0h+var_6D0+18h]
                vpmaxud ymm6, ymm6, ymm3
                vpsubd  ymm3, ymm6, ymm3
                vpmaxud xmm6, xmm3, xmm4
                vpcmpeqd xmm6, xmm3, xmm6
                vpshufb xmm6, xmm6, cs:__xmm@0000000000000000800c800880048000
                vpsrlw  xmm5, xmm5, 8
                vpackuswb xmm5, xmm5, xmm6
                vpmovmskb ecx, xmm5
                and     ecx, 0FFFh
                vmovdqa ymmword ptr [r14+202020h], ymm3 ; res.lens[8..15]
                popcnt  ecx, ecx                        ; novf, as a popcount of the packed compare masks
                vextracti128 xmm3, ymm3, 1
                vpmaxud xmm4, xmm3, xmm4
                vpcmpeqd xmm3, xmm3, xmm4
                vmovmskps edx, xmm3
                mov     r8d, edx
                shr     r8b, 3
                mov     r9d, edx
                and     r9b, 4
                shr     r9b, 2
                mov     r10d, edx
                and     r10b, 2
                shr     r10b, 1
                and     edx, 1
                movzx   r10d, r10b
                movzx   r9d, r9b
                add     r9d, r10d
                add     r9d, edx
                add     r9d, ecx
                movzx   ecx, r8b
                or      ecx, r9d
                mov     rsi, [rsp+7F0h+var_798]
                jnz     loc_1400C2882                   ; any lane overflowed -> rc_overflow(), which exits
                vpinsrd xmm3, xmm0, eax, 2              ; \ ncarry = AND of all 16 FFNum lanes, as a log2 tree.  The
                vpinsrd xmm3, xmm3, dword ptr [rsp+7F0h+var_6D0+1Ch], 3
                vinserti128 ymm2, ymm2, xmm3, 1
                vpbroadcastq ymm1, xmm1
                vpblendd ymm1, ymm2, ymm1, 30h ; '0'
                vpand   ymm0, ymm0, ymm1                ;  |  mask form (FFNum &= _cy-1) is why this is a plain vpand
                vextracti128 xmm1, ymm0, 1
                vpand   xmm0, xmm0, xmm1
                vpshufd xmm1, xmm0, 0EEh
                vpand   xmm0, xmm0, xmm1
                vpshufd xmm1, xmm0, 55h ; 'U'
                vpand   xmm0, xmm0, xmm1                ; /  reduction with no +1 fixup
                vmovd   eax, xmm0
                test    eax, eax
                jz      short loc_1400C24F5             ; some lane lost a carry -> re-code the block with the twin
                mov     this, r13       ; C
                mov     rdx, r14        ; rc
                mov     r8, [rsp+7F0h+var_670]
                vzeroupper
                call    RCio_65536_16___rc_Write_Model_0___t_res_  ; no lane did -> emit the 16 substreams
                jmp     loc_1400C0A32
; ---------------------------------------------------------------------------

loc_1400C24F5:                          ; CODE XREF: Model_0___do_process+2118^j
                mov     this, r14       ; blksize
                mov     rdx, [rsp+7F0h+var_7A0]
                vzeroupper
                call    Model_0___recode_with_carry
                jmp     loc_1400C0A32
; ---------------------------------------------------------------------------

;=============================================================================
;  +214Ah .. +2332h   -C's rc_Quit x16, folds and rc_Write.  COLD.
;  Same three steps as the vector path above, but the 16 rc_Quit's are real
;  calls and the two folds are 16-term scalar add chains over rcC's lanes:
;  +38h is the lost-carry counter, +18h is f_OVF.
;=============================================================================
loc_1400C250A:                          ; CODE XREF: Model_0___do_process+17B4^j
                xor     ecx, ecx
                cmp     ecx, eax
                jb      loc_1400C1C9D
                db      66h, 66h, 2Eh
                nop     word ptr [rax+rax+00000000h]

loc_1400C2520:                          ; CODE XREF: Model_0___do_process+18D7^j
                                        ; Model_0___do_process+1931^j
                mov     this, [rsp+7F0h+var_780]
                call    Rangecoder_CL_0___rc_Quit
                mov     this, [rsp+7F0h+var_418]
                call    Rangecoder_CL_0___rc_Quit
                mov     this, [rsp+7F0h+var_440]
                call    Rangecoder_CL_0___rc_Quit
                mov     this, [rsp+7F0h+var_468]
                call    Rangecoder_CL_0___rc_Quit
                mov     this, [rsp+7F0h+var_490]
                call    Rangecoder_CL_0___rc_Quit
                mov     this, [rsp+7F0h+var_4B8]
                call    Rangecoder_CL_0___rc_Quit
                mov     this, [rsp+7F0h+var_4E0]
                call    Rangecoder_CL_0___rc_Quit
                mov     this, [rsp+7F0h+var_508]
                call    Rangecoder_CL_0___rc_Quit
                mov     this, [rsp+7F0h+var_530]
                call    Rangecoder_CL_0___rc_Quit
                mov     this, [rsp+7F0h+var_558]
                call    Rangecoder_CL_0___rc_Quit
                mov     this, [rsp+7F0h+var_580]
                call    Rangecoder_CL_0___rc_Quit
                mov     this, [rsp+7F0h+var_5A8]
                call    Rangecoder_CL_0___rc_Quit
                mov     this, [rsp+7F0h+var_5D0]
                call    Rangecoder_CL_0___rc_Quit
                mov     this, [rsp+7F0h+var_5F8]
                call    Rangecoder_CL_0___rc_Quit
                mov     this, [rsp+7F0h+var_620]
                call    Rangecoder_CL_0___rc_Quit
                mov     this, [rsp+7F0h+var_648]
                call    Rangecoder_CL_0___rc_Quit
                mov     eax, [r14+202078h]
                add     eax, [r14+2020B8h]
                add     eax, [r14+2020F8h]
                add     eax, [r14+202138h]
                add     eax, [r14+202178h]
                add     eax, [r14+2021B8h]
                add     eax, [r14+2021F8h]
                add     eax, [r14+202238h]
                add     eax, [r14+202278h]
                add     eax, [r14+2022B8h]
                add     eax, [r14+2022F8h]
                add     eax, [r14+202338h]
                add     eax, [r14+202378h]
                add     eax, [r14+2023B8h]
                add     eax, [r14+2023F8h]
                add     eax, [r14+202438h]
                jnz     loc_1400C0A20                   ; any carry lost -> recode_with_carry
                mov     eax, [r14+202058h]
                add     eax, [r14+202098h]
                add     eax, [r14+2020D8h]
                add     eax, [r14+202118h]
                add     eax, [r14+202158h]
                add     eax, [r14+202198h]
                add     eax, [r14+2021D8h]
                add     eax, [r14+202218h]
                add     eax, [r14+202258h]
                add     eax, [r14+202298h]
                add     eax, [r14+2022D8h]
                add     eax, [r14+202318h]
                add     eax, [r14+202358h]
                add     eax, [r14+202398h]
                add     eax, [r14+2023D8h]
                add     eax, [r14+202418h]
                mov     rsi, [rsp+7F0h+var_798]
                jnz     loc_1400C2882                   ; any lane overflowed -> rc_overflow()
                mov     this, r13       ; C
                mov     rdx, r14        ; rc
                mov     r8, [rsp+7F0h+var_780]
                call    RCio_65536_16___rc_Write_RangecoderN_65536_16_Rangecoder_CL_0_____
                jmp     loc_1400C0A32
; ---------------------------------------------------------------------------

loc_1400C26F3:                          ; CODE XREF: Model_0___do_process+1C65^j
                add     rdx, this
                jmp     loc_1400C2799
; ---------------------------------------------------------------------------
                align 20h

;=============================================================================
;  +2340h  VECTOR TAIL REMAINDER -- one bit at a time, straight out of the
;  lane arrays at [rsp + (i%16)*4].  Entered when fewer than 8 bits are left
;  or the 8-wide window would wrap.  At most 15 bits per file.
;=============================================================================
loc_1400C2700:                          ; CODE XREF: Model_0___do_process+2441vj
                                        ; Model_0___do_process+244Fvj
                mov     dword ptr [rsp+r8+7F0h+var_6F0], edi
                mov     dword ptr [rsp+r8+7F0h+var_770], r10d
                mov     edi, dword ptr [rsp+r8+7F0h+var_2B0]
                lea     ebx, [rdi-2]
                mov     dword ptr [rsp+r8+7F0h+var_330], ebx
                sub     edi, r15d
                mov     dword ptr [rsp+r8+7F0h+var_2B0], edi
                mov     edi, r14d
                shr     edi, 10h
                mov     r15b, 10h
                sub     r15b, sil
                shrx    edi, edi, r15d
                shlx    r15d, r10d, esi
                or      r15d, edi
                mov     dword ptr [rsp+r8+7F0h+var_2F0], r15d
                shlx    edi, r14d, esi
                mov     dword ptr [rsp+r8+7F0h+var_370], edi
                shlx    r11d, r11d, esi
                mov     esi, r11d
                shr     esi, 0Fh
                imul    esi, r9d
                sub     r11d, esi
                test    ecx, ecx
                cmovz   r11d, esi
                mov     dword ptr [rsp+r8+7F0h+var_3F0], r11d
                neg     ecx
                and     ecx, esi
                mov     dword ptr [rsp+r8+7F0h+var_3B0], ecx
                mov     [r13+rbx+0], r10d
                inc     rdx
                cmp     rax, rdx
                jz      loc_1400C2030

loc_1400C2799:                          ; CODE XREF: Model_0___do_process+1A0D^j
                                        ; Model_0___do_process+1A2D^j ...
                mov     this, [rsp+7F0h+var_790]
                movzx   ecx, word ptr [this+rdx*2]
                mov     r9d, ecx
                and     r9d, 7FFFh
                shr     ecx, 0Fh
                mov     r8d, edx
                and     r8d, 0Fh
                shl     r8d, 2
                mov     r11d, dword ptr [rsp+r8+7F0h+var_3F0]
                xor     r15d, r15d
                cmp     r11d, 1000000h
                setb    r15b
                cmp     r11d, 10000h
                adc     r15d, 0
                lea     esi, ds:0[r15*8]
                mov     r14d, dword ptr [rsp+r8+7F0h+var_3B0]
                xor     edi, edi
                add     r14d, dword ptr [rsp+r8+7F0h+var_370]
                mov     r10d, dword ptr [rsp+r8+7F0h+var_2F0]
                adc     r10d, 0
                jb      loc_1400C2700
                mov     edi, dword ptr [rsp+r8+7F0h+var_6F0]
                jmp     loc_1400C2700
; ---------------------------------------------------------------------------

;=============================================================================
;  +2454h  EPILOGUE.  yield(this,0) tells the coroutine the stream is done,
;  then the 10 xmm restores the MS ABI wants and the frame teardown.
;=============================================================================
loc_1400C2814:                          ; CODE XREF: Model_0___do_process+67A^j
                mov     this, r14       ; p
                xor     edx, edx        ; value
                call    yield_0
                vmovaps xmm6, [rbp+770h+var_E0]
                vmovaps xmm7, [rbp+770h+var_D0]
                vmovaps xmm8, [rbp+770h+var_C0]
                vmovaps xmm9, [rbp+770h+var_B0]
                vmovaps xmm10, [rbp+770h+var_A0]
                vmovaps xmm11, [rbp+770h+var_90]
                vmovaps xmm12, [rbp+770h+var_80]
                vmovaps xmm13, [rbp+770h+var_70]
                vmovaps xmm14, [rbp+770h+var_60]
                vmovaps xmm15, [rbp+770h+var_50]
                lea     rsp, [rbp+738h]
                pop     rbx
                pop     rdi
                pop     rsi
                pop     r12
                pop     r13
                pop     r14
                pop     r15
                pop     rbp
                retn
; ---------------------------------------------------------------------------

;=============================================================================
;  +24C2h  rc_overflow -- a substream did not fit its row, so the 2-byte
;  length header cannot describe it either.  Prints and exits; unreachable
;  for any BLKSIZE/RCNUM whose worst case fits 0FFFFh.
;=============================================================================
loc_1400C2882:                          ; CODE XREF: Model_0___do_process+20CA^j
                                        ; Model_0___do_process+2318^j
                vzeroupper
                call    Model_0___rc_overflow
; ---------------------------------------------------------------------------
                db 0CCh
Model_0___do_process endp

