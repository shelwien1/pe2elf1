// sub_10006E31 @ 0x10006E31 -- hand-written (incdec.md §4.1): the same walk as
// sub_10006CB0 with a different emit, the right child always at +4, and EDX
// holding a byte table instead of an offset.
//   emit(v): [out+4] = v; if table[(-v) >> 3] != dword_10016488 then
//            [out] = ESI, advance by 8, and end the search when the count runs out.
// patch: __thunk_sub_10006E31
static int32_t &walk2_skip = *(int32_t *)0x10016488;     // dword_10016488

static inline void walk2_emit(int32_t v, VbnWalk *w)
{
  *(int32_t *)(w->out + 4) = v;
  uint32_t t = (0u - (uint32_t)v) >> 3;                   // neg ebx; shr ebx,3
  if ((uint32_t)((uint8_t *)(uintptr_t)w->aux)[t] != (uint32_t)walk2_skip) {
    *(int32_t *)w->out = w->value;
    w->out += 8;
    if (--w->left == 0)
      vbn_walk_exit();
  }
}
static void walk2(int32_t *node, VbnWalk *w)
{
  int32_t l = node[0];
  if (l > 0)
    walk2((int32_t *)(uintptr_t)l, w);
  else
    walk2_emit(l, w);
  int32_t r = node[1];
  if (r > 0)
    walk2((int32_t *)(uintptr_t)r, w);
  else if (node[0] > (int32_t)(uintptr_t)node)    // cmp eax,[eax]; jle
    walk2_emit(r, w);
}
PROBE_DECL(__sub_10006E31)
extern "C" __attribute__((used, visibility("hidden"), force_align_arg_pointer))
void vbn_walk2_entry(int32_t *node, VbnWalk *w)
{
  PROBE_HIT(__sub_10006E31);
  walk2(node, w);
}
extern "C" __attribute__((naked, used, visibility("hidden"))) void __thunk_sub_10006E31()
{
  __asm__ volatile(
    "push %esi\n"
    "push %edx\n"
    "push %edi\n"
    "push %ecx\n"
    "mov %esp, %eax\n"
    "push %eax\n"
    "push %ebx\n"
    "call vbn_walk2_entry\n"
    "add $8, %esp\n"
    "pop %ecx\n"
    "pop %edi\n"
    "pop %edx\n"
    "pop %esi\n"
    "ret\n");
}
