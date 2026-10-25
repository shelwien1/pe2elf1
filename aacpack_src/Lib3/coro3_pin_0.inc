
#pragma once

struct coro3_pin_0 {
  byte* ptr;
  byte* beg;
  byte* end;
  uint  f_EOF;
  word  base_offs;
  word  r_code;

  uint getoutsize() { return ptr-beg; } //-V110

  void addinp( byte* inp,uint inplen ) { addbuf(inp,inplen); }
  void addout( byte* out,uint outlen ) { addbuf(out,outlen); }

  void addbuf( byte* buf,uint len ) {
    beg = ptr = buf;
    end = &buf[len];
  }

};
