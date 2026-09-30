// wdump: tfwc2 <-> flat dump (for numpy analysis)
//   wdump x in.tfwc2 out.wd     dump all tensors in file order (rope tables: shape only)
//   wdump p in.wd out.tfwc2     pack back to FX2TFWC2
#include <cassert>
#include <cmath>
#include <cstdarg>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <algorithm>
#include <initializer_list>
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include "../tfwz/tf/weights_io.h"
#include "../tfwz/tf/weights_io.inc"
#include "../tfwz/tf/weights_io_compressed.inc"
#include "../tfwz/tf/weights_write.inc"
using namespace fx2;
static void wr(FILE* f, const void* p, size_t n) { if (fwrite(p, 1, n, f) != n) { fprintf(stderr, "write error\n"); exit(1); } }
static void rd(FILE* f, void* p, size_t n) { if (fread(p, 1, n, f) != n) { fprintf(stderr, "read error\n"); exit(1); } }
int main(int argc, char** argv) {
  if (argc != 4) { fprintf(stderr, "usage: wdump x|p in out\n"); return 1; }
  if (argv[1][0] == 'x') {
    WeightsFile wf = WeightsFile::load_compressed(argv[2]);
    FILE* f = fopen(argv[3], "wb");
    wr(f, "WDMP", 4);
    uint32_t n = wf.order.size(); wr(f, &n, 4);
    for (auto& name : wf.order) {
      const WTensor& t = wf.get(name);
      uint32_t nl = name.size(); wr(f, &nl, 4); wr(f, name.data(), nl);
      wr(f, &t.dtype, 1);
      uint32_t nd = t.shape.size(); wr(f, &nd, 4);
      for (auto d : t.shape) wr(f, &d, 4);
      bool rope = name == "rope.sin" || name == "rope.cos";
      uint64_t nb = rope ? 0 : t.data.size(); wr(f, &nb, 8);
      wr(f, t.data.data(), nb);
    }
    fclose(f);
  } else {
    FILE* f = fopen(argv[2], "rb");
    char m[4]; rd(f, m, 4);
    uint32_t n; rd(f, &n, 4);
    WeightsFile wf;
    for (uint32_t i = 0; i < n; i++) {
      uint32_t nl; rd(f, &nl, 4); std::string name(nl, 0); rd(f, &name[0], nl);
      WTensor t; rd(f, &t.dtype, 1);
      uint32_t nd; rd(f, &nd, 4); t.shape.resize(nd); t.numel = 1;
      for (auto& d : t.shape) { rd(f, &d, 4); t.numel *= d; }
      uint64_t nb; rd(f, &nb, 8); t.data.resize(nb); rd(f, t.data.data(), nb);
      wf.order.push_back(name);
      wf.tensors[name] = std::move(t);
    }
    fclose(f);
    if (!write_weights_v2(argv[3], wf, wf.order)) return 1;
  }
  return 0;
}
