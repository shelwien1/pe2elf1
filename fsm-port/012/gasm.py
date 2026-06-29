#!/usr/bin/env python3
# gasm.py -- generate x86_tables_enc.h, the ENCODE side of the corpus.
#
#   corpus.p --gen.py--> x86_tables.h        (decode: bytes -> x86insn_t)
#   corpus.p --gasm.py--> x86_tables_enc.h    (encode: x86insn_t -> bytes)
#
# This is a SEPARATE program from gen.py, but it reuses gen.py's corpus parser
# and the same per-rule derivation (parse_insn_lhs / insn_mnem / operand_file),
# so the two headers cannot drift in how a rule is interpreted. Decode bakes a
# per-byte FSM; encode bakes the inverse map keyed by mnemonic: a list of
# candidate encodings, each carrying the opcode byte(s), the embedded-field
# program, the operand-file signature and the immediate kind. The C++ encoder
# selects a candidate by (mnemonic, operand signature) refined by the `enc`
# witness, then emits ModR/M/SIB/disp and the immediate.
#
# It depends on x86_tables.h only indirectly: the enum *values* (FORM_*, IMK_*,
# OPF_*, GREG_*) are the SAME integers gen.py emits there, because both read
# them from the one Interp; the generated header #includes x86_tables.h so the
# C++ side shares those enums literally.

import sys
import gen


# --------------------------------------------------------------------------
# candidate construction: merge corpus rules per (two_byte, opcode, digit),
# exactly as gen.build_insn merges them per decode byte.
# --------------------------------------------------------------------------
class EncBuilder:
  def __init__(self, interp):
    self.ip = interp
    self.F = {n: i for i, n in enumerate(interp.INSN_FORM)}
    self.K = {n: i for i, n in enumerate(interp.INSN_IMK)}
    self.OPF = {n: i for i, n in enumerate(interp.OPF)}
    self.mnem = interp._mnem                      # mnemonic string table (indices match decode)
    self.cands = {}                               # key (tb,op,digit) -> candidate dict
    self.SKIP = ('m10', 'elt')                    # reptype / 3DNow-ish families: deferred in gen too

  def midx(self, s):
    return self.mnem.index(s)

  def cand(self, key):
    return self.cands.setdefault(key, dict(
        tb=key[0], op=key[1], digit=key[2],
        mnem=None, form=self.F['NONE'], imk=self.K['NONE'],
        dir=0, rfile=self.OPF['GREG'], mfile=self.OPF['GREG'],
        sfx=0, emb='none', emb_pos=0, emb_w=0,
        reg0=0, sup_reg=0, sup_mem=0))

  # ---- mirror of gen.build_insn's per-rule attribute derivation ----
  def add_rule(self, lhs, rhs):
    ip = self.ip
    info = ip.parse_insn_lhs(lhs)
    if info is None:
      return
    if info['modrm'] in ('group_reg', 'group_mem'):    # groups: from _group_list later
      return
    if 'elt[' in rhs:                                   # ssef element families: deferred (as in gen)
      return
    mn = ip.insn_mnem(rhs)
    if mn[0] == 'sel' and mn[1] in self.SKIP:
      return
    bp, emb, imml = info['bp'], info['embedded'], info['imm']
    tb = info['tb']                                    # 0 one-byte, 1 0F, 2 0F 38, 3 0F 3A
    F, K, OPF = self.F, self.K, self.OPF

    # immediate kind (imm8 may be sign-extended -- detect from RHS)
    if imml == ['imm32', 'imm16']:
      imk = K['PTR']
    elif imml == ['imm16', 'imm8']:
      imk = K['ENTER']
    elif len(imml) == 1:
      imk = K['IMM8SX'] if (imml[0] == 'imm8' and 'sx8(' in rhs) else K[imml[0].upper()]
    elif not imml:
      imk = K['NONE']
    else:
      return

    has_g = bool(gen.re.search(r'(greg|rgb|sreg|ssereg)\[[^\]]*\$g[^\]]*\]', rhs))
    two_ops = (',' in rhs)
    if info['modrm'] in ('reg', 'mem'):
      form = F['MODRM'] if has_g else F['RM']
    elif emb == 'b' and imml:
      form = F['REG_IMM']
    elif emb == 'b':
      form = F['REG']
    elif imk in (K['PTR'], K['ENTER']):
      form = F['PTR']
    elif imml and imml[0] in ('rel8', 'relz'):
      form = F['REL']
    elif imml and two_ops:
      form = F['ACC']                                  # implicit accumulator (eAX/al, imm)
    elif imml:
      form = F['IMM']
    else:
      form = F['NONE']

    dir_rm = 0
    if form == F['MODRM']:
      gpos, rmpos = rhs.find('$g'), rhs.find('$addr')
      if rmpos < 0:
        rmpos = rhs.find('$r')
      dir_rm = 1 if (gpos >= 0 and rmpos >= 0 and gpos > rmpos) else 0

    sfx = 0
    if info['modrm'] == 'mem':
      sm = gen.re.search(r'sfx\[(\d+)\]', rhs)
      if sm:
        sfx = int(sm.group(1))

    # mnemonic list: one for lit/cond, one per table entry for an opsize-selected
    # 'sel' (uc/m54/movs). The opsize that picks the entry rides as the 66 prefix,
    # which the encoder replays -- so each resolved mnemonic just maps to this opcode.
    cc_emb = False
    if mn[0] == 'lit':
      mnem_list = [self.midx(mn[1])]
    elif mn[0] == 'cond':
      mnem_list = [self.midx(mn[1] + 'cc')]            # jcc / setcc / cmovcc
      cc_emb = True
    else:                                              # ('sel', table, var)
      mnem_list = [self.midx(n) for n in ip.tables[mn[1]]]

    for mnem_idx in mnem_list:
      key = (tb, bp.lit_val, 0xFF, mnem_idx)
      c = self.cand(key)
      c['mnem'] = mnem_idx
      if form != F['NONE']:
        c['form'] = form
      if imk != K['NONE']:
        c['imk'] = imk
      if dir_rm:
        c['dir'] = 1
      if emb in ('b', 'c'):
        pos, w = bp.fields[emb]
        c['emb'] = 'cc' if (cc_emb or emb == 'c') else 'reg'
        c['emb_pos'], c['emb_w'] = pos, w
      elif cc_emb:
        c['emb'] = 'cc'
      if form == F['ACC']:
        c['reg0'] = 1                                  # implicit eAX / al
      if info['modrm'] == 'reg':
        c['rfile'] = OPF[ip.operand_file(rhs, 'g')]
        c['mfile'] = OPF[ip.operand_file(rhs, 'r')]
        c['sup_reg'] = 1
      elif info['modrm'] == 'mem':
        c['rfile'] = OPF[ip.operand_file(rhs, 'g')]
        c['sup_mem'] = 1
        if sfx:
          c['sfx'] = sfx
      elif emb == 'b':
        c['rfile'] = OPF[ip.operand_file(rhs, 'b')]

  # ---- opcode-extension groups, from gen's _group_list ----
  def add_groups(self):
    F, K = self.F, self.K
    for gid, (tb, opcode, members) in enumerate(self.ip._group_list):
      for reg, m in members.items():
        for which, mn in (('reg', m['mnem_reg']), ('mem', m['mnem_mem'])):
          if mn is None:
            continue
          key = (tb, opcode, reg if which == 'reg' else 0x100 + reg)  # distinct keys reg/mem
          c = self.cand(key)
          c['digit'] = reg
          c['mnem'] = mn
          c['form'] = F['GROUP']
          c['imk'] = m['imk']
          if which == 'reg':
            c['sup_reg'] = 1
          else:
            c['sup_mem'] = 1

  def build(self):
    for lhs, rhs in self.ip.insn_rules_raw():
      self.add_rule(lhs, rhs)
    self.add_groups()
    # bucket candidates by mnemonic, contiguous, stable order
    by_mnem = {}
    for key in sorted(self.cands):
      c = self.cands[key]
      by_mnem.setdefault(c['mnem'], []).append(c)
    # ---- enc-field capacity guard --------------------------------------------
    # enc is 3 bits (x86insn_t.enc): at most 8 candidates may share a runtime
    # operand shape under one mnemonic, since the decoder disambiguates equal-
    # shape encodings by recording the candidate rank (0..7) in `enc`.  Candidates
    # that pass enc_cand_roles together compete for that rank space; group them by
    # the shape signature it keys on and fail loudly if any group exceeds 8.
    ENC_CAP = 1 << 3
    for mnem, cs in by_mnem.items():
      shape = {}
      for c in cs:
        # candidates compete for enc-rank only if they can match the same insn:
        # same form, operand files, immediate kind, and r/m capability (reg vs mem).
        k = (c['form'], c['rfile'], c['mfile'], c['imk'], c['sup_reg'], c['sup_mem'])
        shape.setdefault(k, []).append(c)
      for k, grp in shape.items():
        if len(grp) > ENC_CAP:
          name = self.ip._mnem[mnem] if mnem < len(self.ip._mnem) else '?'
          raise SystemExit(
            "gasm.py: enc overflow -- mnemonic %r has %d same-shape encodings "
            "(cap %d). Give some of these opcodes distinct mnemonics. shape=%r"
            % (name, len(grp), ENC_CAP, k))
    return by_mnem


# --------------------------------------------------------------------------
# emit x86_tables_enc.h
# --------------------------------------------------------------------------
EMB = {'none': 'EMB_NONE', 'reg': 'EMB_REG', 'cc': 'EMB_CC'}


def cand_str(c):
  return ("{%d,%d,0x%02x,%s,%d,%d,%d,%d,%d,%d,%s,%s,%d,%d,%d,%d}" % (
      c['mnem'], c['tb'], c['op'], EMB[c['emb']], c['emb_pos'], c['emb_w'],
      (c['digit'] if c['digit'] != 0xFF else 0xFF),
      c['form'], c['imk'], c['dir'],
      'OPF_' + Interp_OPF[c['rfile']], 'OPF_' + Interp_OPF[c['mfile']],
      c['sfx'], c['reg0'], c['sup_reg'], c['sup_mem']))


Interp_OPF = gen.Interp.OPF


def emit(interp, by_mnem, out_path):
  nmnem = len(interp._mnem)
  o = []
  w = o.append
  w("// Generated from corpus.p by gasm.py. Do not edit.\n")
  w("// Encode side: x86insn_t -> bytes (inverse of x86_tables.h's decode FSM).\n")
  w("#ifndef X86_TABLES_ENC_H\n#define X86_TABLES_ENC_H\n\n")
  w('#include "x86_tables.h"   // shares FORM_*/IMK_*/OPF_*/GREG_* enum values\n\n')

  w("enum EmbKind { EMB_NONE = 0, EMB_REG, EMB_CC };\n")
  w("// operand classes for candidate signature matching (derived from x86op_t.type)\n")
  w("enum OpClass { C_NONE = 0, C_GREG, C_RGB, C_XMM, C_MM, C_SREG, C_MEM, C_IMM, C_REL };\n\n")

  w("// one encoding candidate: opcode byte(s) + embedded-field program + signature.\n")
  w("struct EncCand {\n")
  w("  uint16_t mnem;            // index into mnem_tab (matches decode)\n")
  w("  uint8_t  tb;              // opcode map: 0 one-byte, 1 0F, 2 0F 38, 3 0F 3A\n")
  w("  uint8_t  op;              // base opcode byte (embedded field bits are 0)\n")
  w("  uint8_t  emb;             // EmbKind: embed reg / cc into the opcode byte\n")
  w("  uint8_t  emb_pos, emb_w;  // bit position / width of the embedded field\n")
  w("  uint8_t  digit;           // group /digit (ModR/M reg field), or 0xFF\n")
  w("  uint8_t  form;            // InsnForm\n")
  w("  uint8_t  imk;             // ImmKind of the trailing immediate\n")
  w("  uint8_t  dir;             // ModR/M order: 0 = reg,r/m   1 = r/m,reg\n")
  w("  uint8_t  rfile, mfile;    // OperandFile of the reg / r-m-direct operand\n")
  w("  uint8_t  sfx;             // memory-form size suffix (unused by encoder, kept for parity)\n")
  w("  uint8_t  reg0;            // implicit accumulator (reg operand must be index 0)\n")
  w("  uint8_t  sup_reg, sup_mem;// r/m slot accepts register-direct / memory\n")
  w("};\n\n")

  flat = []
  starts = [0] * nmnem
  counts = [0] * nmnem
  for m in range(nmnem):
    starts[m] = len(flat)
    for c in by_mnem.get(m, []):
      flat.append(c)
    counts[m] = len(flat) - starts[m]

  w("static const struct EncCand enc_cand[] = {\n")
  for i, c in enumerate(flat):
    w("  %s,  // %3d  %s\n" % (cand_str(c), i, interp._mnem[c['mnem']]))
  if not flat:
    w("  {0,0,0,EMB_NONE,0,0,0xFF,0,0,0,OPF_GREG,OPF_GREG,0,0,0,0}\n")
  w("};\n")
  w("static const size_t enc_cand_count = sizeof(enc_cand)/sizeof(enc_cand[0]);\n\n")

  w("// per-mnemonic bucket: candidates for mnem M are enc_cand[start..start+count).\n")
  w("static const uint16_t enc_bucket_start[] = {\n    %s\n};\n" %
    ", ".join(map(str, starts)))
  w("static const uint16_t enc_bucket_count[] = {\n    %s\n};\n" %
    ", ".join(map(str, counts)))
  w("static const size_t enc_nmnem = %d;\n\n" % nmnem)

  w("#endif // X86_TABLES_ENC_H\n")
  open(out_path, 'w', encoding='utf-8').write(''.join(o))


def main():
  corpus = sys.argv[1] if len(sys.argv) > 1 else 'corpus.p'
  out = sys.argv[2] if len(sys.argv) > 2 else 'x86_tables_enc.h'
  c = gen.parse_corpus(corpus)
  interp = gen.Interp(c)
  by_mnem = EncBuilder(interp).build()
  emit(interp, by_mnem, out)
  ncand = sum(len(v) for v in by_mnem.values())
  print("gasm.py: wrote %s  (%d candidates across %d mnemonics)" %
        (out, ncand, len(interp._mnem)))


if __name__ == '__main__':
  main()
