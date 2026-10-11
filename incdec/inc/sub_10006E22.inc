// sub_10006E22 @ 0x10006E22 -- hand-written (incdec.md §4.1): the longjmp half of the
// compressor's early exit:  mov esp,[10016474]; mov ebp,[10016478]; ret 4.
// Returns from the call that followed sub_1000703B as if it had finished.
// patch: __sub_10006E22
PROBE_DECL(__sub_10006E22)
extern "C" __attribute__((used, visibility("hidden"), force_align_arg_pointer)) void vbn_probe_10006E22()
{ PROBE_HIT(__sub_10006E22); }
extern "C" __attribute__((naked, used, visibility("hidden"))) void __sub_10006E22()
{
  __asm__ volatile(
    "pushal\n"
    "call vbn_probe_10006E22\n"
    "popal\n"
    "movl 0x10016474, %esp\n"
    "movl 0x10016478, %ebp\n"
    "ret $4\n");
}
