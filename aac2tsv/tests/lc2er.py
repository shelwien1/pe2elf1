#!/usr/bin/env python3
"""lc2er.py BIN in.aac out.aus : AAC LC ADTS rewritten as ER AAC LC (AUS1) through the TSV: aac2tsv c, one access
unit per raw_data_block, AudioSpecificConfig with object type 17, channel elements without element ids, END,
PCE, DSE and FIL, aac2tsv d. The result decodes to the same audio as the input, which cross-checks the ER element
syntax against decoders. Needs streams whose raw_data_blocks hold exactly the channel elements of one channel
configuration (plus PCE/DSE/FIL); with channel configuration 0 it is taken from the first block."""
import sys, subprocess
binp, src, out = sys.argv[1:4]
tsv = out + '.tsv'
subprocess.run([binp, 'c', '-q', src, tsv], check=True)
layouts = {'S': 1, 'C': 2, 'SC': 3, 'SCS': 4, 'SCC': 5, 'SCCL': 6, 'SCCCL': 7}
inner = {'ics', 'ms', 'sec', 'sf', 'pulse', 'tns', 'spec'}
skip = {'adtsh', 'adts', 'crc', 'rdbpos', 'pce', 'dse', 'fil', 'fill', 'ext', 'drc'}
head, body, els, cfg, chan = [], [], None, None, None
for l in open(tsv).read().split('\n'):
    f = l.split('\t'); r = f[0]
    if r == 'adtsh':
        if cfg is not None and cfg != f[1:]: sys.exit('ADTS configuration changes')
        cfg = f[1:]
        if int(f[4]) != 1: sys.exit('not AAC LC')
    if r in ('sce', 'cpe', 'lfe'):
        if els is None: els = ''; body.append('au')
        els += r[0].upper(); body.append(l)
    elif r == 'end':
        if els not in layouts: sys.exit('raw_data_block elements %s are no ER channel configuration' % els)
        if chan is None: chan = layouts[els]
        if layouts[els] != chan or int(cfg[6]) not in (0, chan): sys.exit('elements %s do not match' % els)
        els = None
    elif r in inner: body.append(l)
    elif r in skip: pass
    elif r in ('aac2tsv', '') or r.startswith('#'): (head if not body else body).append(l)
    elif r == 'eof': body.append(l)   # its size and CRC no longer match: d warns
    else: sys.exit('record %s cannot be rewritten as ER AAC LC' % r)
bits = format(17, '05b') + format(int(cfg[4]), '04b') + format(chan, '04b') + '001' + '000' + '0' + '00'
bits += '0' * (-len(bits) % 8)
asc = bytes(int(bits[i:i + 8], 2) for i in range(0, len(bits), 8))
open(tsv, 'w').write('\n'.join(head + ['aus1\t' + asc.hex()] + body) + '\n')
subprocess.run([binp, 'd', tsv, out], check=True, stderr=subprocess.DEVNULL)
