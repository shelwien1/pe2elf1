// sub_10006CB0 @ 0x10006CB0 -- hand-written (incdec.md §4.1): in-order walk of a
// context subtree, emitting its leaves.  Register interface (assembly):
//   in   EBX node, EDX offset of the right child, ESI value, ECX output cursor,
//        EDI how many pairs may still be written
//   out  ECX, EDI (EDX and ESI are left alone; EAX/EBX are scratch)
// The code is the walk below unrolled two levels deep.  A field <= 0 is a leaf,
// > 0 a child.  emit(v) writes {ESI, v} (v at +offset) and advances by 8; when
// the count runs out the search ends through vbn_walk_exit.  Hex-Rays could not
// follow the register interface (JUMPOUT).
// patch: __thunk_sub_10006CB0
struct VbnWalk { uint8_t *out; int32_t left; int32_t aux; int32_t value; };  // ECX EDI EDX ESI

static inline void walk1_emit(int32_t v, VbnWalk *w)
{
  *(int32_t *)w->out = w->value;
  *(int32_t *)(w->out + w->aux) = v;
  w->out += 8;
  if (--w->left == 0)
    vbn_walk_exit();
}
static void walk1(int32_t *node, VbnWalk *w)
{
  int32_t l = node[0];
  if (l > 0)
    walk1((int32_t *)(uintptr_t)l, w);
  else
    walk1_emit(l, w);
  int32_t r = *(int32_t *)((uint8_t *)node + w->aux);
  if (r > 0)
    walk1((int32_t *)(uintptr_t)r, w);
  else if (node[0] > (int32_t)(uintptr_t)node)    // cmp eax,[eax]; jle
    walk1_emit(r, w);
}
PROBE_DECL(__sub_10006CB0)
extern "C" __attribute__((used, visibility("hidden"), force_align_arg_pointer))
void vbn_walk1_entry(int32_t *node, VbnWalk *w)
{
  PROBE_HIT(__sub_10006CB0);
  walk1(node, w);
}
extern "C" __attribute__((naked, used, visibility("hidden"))) void __thunk_sub_10006CB0()
{
  __asm__ volatile(
    "push %esi\n"
    "push %edx\n"
    "push %edi\n"
    "push %ecx\n"                  // VbnWalk {ECX, EDI, EDX, ESI} at ESP
    "mov %esp, %eax\n"
    "push %eax\n"
    "push %ebx\n"
    "call vbn_walk1_entry\n"
    "add $8, %esp\n"
    "pop %ecx\n"
    "pop %edi\n"
    "pop %edx\n"
    "pop %esi\n"
    "ret\n");
}
