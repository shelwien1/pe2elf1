// sub_1000703B @ 0x1000703B -- hand-written (incdec.md §4.1): the setjmp half of the
// compressor's early exit:  mov [10016474],esp; mov [10016478],ebp; ret 4.
// It records the caller's ESP (where the next call's return address will go)
// and EBP, so it has to be naked; see head.cpp, vbn_restartable.  The probe is
// counted from a C helper between pushal and popal: nothing else changes.
// patch: __sub_1000703B
PROBE_DECL(__sub_1000703B)
extern "C" __attribute__((used, visibility("hidden"), force_align_arg_pointer)) void vbn_probe_1000703B()
{ PROBE_HIT(__sub_1000703B); }
extern "C" __attribute__((naked, used, visibility("hidden"))) void __sub_1000703B()
{
  __asm__ volatile(
    "movl %esp, 0x10016474\n"
    "movl %ebp, 0x10016478\n"
    "pushal\n"
    "call vbn_probe_1000703B\n"
    "popal\n"
    "ret $4\n");
}
