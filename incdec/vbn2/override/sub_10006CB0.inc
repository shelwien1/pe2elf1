// sub_10006CB0 @ 0x10006CB0 -- in-order walk of a context subtree, emitting its
// leaves.  In vbn.exe it is assembly with a register interface (EBX node, EDX
// offset of the right child, ESI value, ECX output cursor, EDI how many pairs
// may still be written; ECX and EDI come back); here its two callers pass a
// VbnWalk.  A field <= 0 is a leaf, > 0 a child.  emit(v) writes {value, v}
// (v at +aux) and advances by 8; when the count runs out the search ends
// through vbn_walk_exit.
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
static void vbn_walk1_entry(int32_t *node, VbnWalk *w)
{
  walk1(node, w);
}
