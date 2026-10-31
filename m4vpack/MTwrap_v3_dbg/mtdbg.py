#!/usr/bin/env python3
"""mtdbg.py -- where a run on threads spent its time, from the dbg.txt of MTwrap_v3_dbg.

    python3 mtdbg.py [dbg.txt] [-w WIDTH] [-r RUN]

dbg.txt is what a build with -DMTWRAP_DBG writes as each run on threads quits (c -t, d of its
streams; c -t -v writes both runs): a row per event of the main loop (thread M) and of each
worker (W<i>); mtdbg.inc has the columns and the events. For each run (or RUN alone) it prints:

  workers   each worker's time: coding (in its coroutine), waiting for input, waiting for room
            for its output (d: while it was its turn to be written, and while another's was),
            ended (before the run did), the rest; its CPU time, bytes and parts
  input     what the main loop was doing while the workers waited for input: stopped by a full
            input ring (whose), reading, writing, the input ended, working
  main      the main loop's time: its waits by cause, its reading and writing
  at once   how many workers were coding at once, for how long
  parts     each part: when its input came, when it was coded, its output written
  timeline  each worker's state and the main loop's, WIDTH columns over the run
  container the runs of one worker's blocks in the container, against an input ring
  dynbuff   d: the blocks put aside, and the most at once
  so        where the time went, in a few lines
"""

import argparse
import bisect
import collections
import csv
import sys
import textwrap

IDS = '0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ'


def wid(w):
    """a worker as one character"""
    return IDS[w] if 0 <= w < len(IDS) else '#'


def mb(n):
    return n / 1048576.0


class Ev(object):
    __slots__ = ('th', 'ev', 't', 'dur', 'w', 'n', 'x', 'note')

    @property
    def end(self):
        return self.t + self.dur


def load(path):
    """the runs in dbg.txt: {run: (mode, [Ev...])}, in time order"""
    runs = collections.OrderedDict()
    with open(path, newline='') as f:
        rd = csv.reader(f, delimiter='\t')
        head = next(rd)
        c = dict((name, i) for i, name in enumerate(head))
        for row in rd:
            if len(row) < len(head) - 1:
                continue
            e = Ev()
            e.th = row[c['thread']]
            e.ev = row[c['event']]
            e.t = float(row[c['t_us']]) / 1e6
            e.dur = float(row[c['dur_us']]) / 1e6
            e.w = int(row[c['worker']])
            e.n = float(row[c['bytes']])
            e.x = int(row[c['x']])
            e.note = row[c['note']] if 'note' in c and c['note'] < len(row) else ''
            r = int(row[c['run']])
            if r not in runs:
                runs[r] = (row[c['mode']], [])
            runs[r][1].append(e)
    for r in runs:
        runs[r][1].sort(key=lambda e: e.t)
    return runs


class Steps(object):
    """a value that changes at times: at(t), and the pieces of [a, b) by value"""

    def __init__(self, t0, v0):
        self.t, self.v = [t0], [v0]

    def set(self, t, v):
        if t <= self.t[-1]:
            self.v[-1] = v
        else:
            self.t.append(t)
            self.v.append(v)

    def at(self, t):
        return self.v[max(bisect.bisect_right(self.t, t) - 1, 0)]

    def pieces(self, a, b):
        k = max(bisect.bisect_right(self.t, a) - 1, 0)
        while a < b:
            nxt = self.t[k + 1] if k + 1 < len(self.t) else float('inf')
            e = min(b, nxt)
            yield a, e, self.v[k]
            a, k = e, k + 1


class Spans(object):
    """disjoint labelled intervals in time order: the overlap of [a, b) with each label"""

    def __init__(self, spans):
        self.s = sorted(spans, key=lambda s: (s[0], s[1]))
        self.ends = [e for _, e, _ in self.s]

    def overlap(self, a, b):
        out = collections.Counter()
        k = bisect.bisect_right(self.ends, a)
        while k < len(self.s) and self.s[k][0] < b:
            s, e, lab = self.s[k]
            lo, hi = max(a, s), min(b, e)
            if hi > lo:
                out[lab] += hi - lo
            k += 1
        return out


class Timeline(object):
    """the time in each state per column, for a row of the chart"""

    def __init__(self, width, T):
        self.width, self.bw = width, T / width if T > 0 else 1.0
        self.col = [collections.Counter() for _ in range(width)]

    def add(self, a, b, s):
        if b <= a:
            return
        k0, k1 = int(a / self.bw), int(b / self.bw)
        for k in range(max(k0, 0), min(k1, self.width - 1) + 1):
            lo, hi = max(a, k * self.bw), min(b, (k + 1) * self.bw)
            if hi > lo:
                self.col[k][s] += hi - lo

    def chars(self, rest):
        """each column's state of the most time (rest: what fills the column's time otherwise)"""
        out = []
        for c in self.col:
            r = self.bw - sum(c.values())
            if r > 0:
                c = c.copy()
                c[rest] += r
            out.append(max(sorted(c), key=lambda s: c[s]) if c else rest)
        return ''.join(out)


def pct(a, b):
    return 100.0 * a / b if b > 0 else 0.0


def say(text, indent=2):
    """a line of the summary, wrapped"""
    print(textwrap.fill(text, 100, initial_indent=' ' * indent, subsequent_indent=' ' * (indent + 2)))


def analyse(run, mode, ev, width):
    dec = mode == 'd'
    P = dict((e.note, int(e.n)) for e in ev if e.ev == 'param')
    ws = sorted(set(e.w for e in ev if e.th.startswith('W')))
    by_th = collections.defaultdict(list)
    for e in ev:
        by_th[e.th].append(e)
    N = P.get('threads') or (max(ws) + 1 if ws else 0)
    T = max([e.t for e in ev if e.ev == 'end'] or [max(e.end for e in ev)])
    ring = P.get('in_slots', 0) * P.get('in_buf', 0)
    M = [e for e in ev if e.th == 'M']
    print('=== run %d: %s on %d threads, %.2f s%s' % (run, 'd' if dec else 'c', N, T,
          '; input rings %d x %d KiB (%.1f MB), output rings %d x %d KiB' % (
              P.get('in_slots', 0), P.get('in_buf', 0) // 1024, mb(ring), P.get('out_slots', 0),
              P.get('out_buf', 0) // 1024) if P else ''))

    # the worker whose output is written (d), or who is dealt the input (c): 0 first, then each round
    j = Steps(0.0, 0)
    for e in M:
        if e.ev == 'round':
            j.set(e.t, e.w)

    # the main loop's states
    mspans = []
    for e in M:
        if e.ev == 'wait':
            lab = ('ring', e.w) if e.x == 1 and e.w >= 0 else 'ended' if e.x == 2 else 'other'
        elif e.ev in ('read', 'write'):
            lab = e.ev
        else:
            continue
        mspans.append((e.t, e.end, lab))
    main = Spans(mspans)

    # the workers
    W = {}
    for w in ws:
        W[w] = dict(code=0.0, win=0.0, wout=0.0, turn=0.0, exit=None, cpu=None, nin=0.0, nout=0.0,
                    parts=0, calls=0, inwhy=collections.Counter(), spans=[])
    for e in (e for w in ws for e in by_th['W%d' % w]):
        s = W[e.w]
        if e.ev == 'call':
            s['code'] += e.dur
            s['calls'] += 1
            s['spans'].append((e.t, e.end, 'R'))
            if e.x == 4:
                s['parts'] += 1
        elif e.ev == 'wait_in':
            s['win'] += e.dur
            s['spans'].append((e.t, e.end, 'i'))
            why = main.overlap(e.t, e.end)
            for lab, d in why.items():
                s['inwhy'][lab] += d
            s['inwhy']['working'] += e.dur - sum(why.values())
        elif e.ev == 'wait_out':
            s['wout'] += e.dur
            for a, b, jw in j.pieces(e.t, e.end):
                mine = dec and jw == e.w
                if mine:
                    s['turn'] += b - a
                s['spans'].append((a, b, 'O' if mine else 'o'))
        elif e.ev == 'pop':
            s['nin'] += e.n
        elif e.ev == 'push':
            s['nout'] += e.n
        elif e.ev == 'exit':
            s['exit'] = e.t
        elif e.ev == 'cpu':
            s['cpu'] = e.dur
    for w in ws:
        s = W[w]
        s['ended'] = T - s['exit'] if s['exit'] is not None else 0.0
        s['rest'] = T - s['code'] - s['win'] - s['wout'] - s['ended']

    print('\nworkers (s; % of the run):' if ws else '\nno workers')
    print('        coding     wait in      wait out (its turn)    ended      rest    cpu   in MB  out MB parts')
    tot = collections.Counter()
    for w in ws:
        s = W[w]
        print('W%-3d %7.2f %3.0f%% %7.2f %3.0f%% %7.2f %3.0f%% %7.2f %7.2f %3.0f%% %6.2f %6.2f %7.1f %7.1f %4d' % (
            w, s['code'], pct(s['code'], T), s['win'], pct(s['win'], T), s['wout'], pct(s['wout'], T),
            s['turn'], s['ended'], pct(s['ended'], T), s['rest'], s['cpu'] or 0.0, mb(s['nin']),
            mb(s['nout']), s['parts']))
        for k in ('code', 'win', 'wout', 'turn', 'ended', 'rest', 'nin', 'nout', 'parts'):
            tot[k] += s[k]
        tot['cpu'] += s['cpu'] or 0.0
    WT = T * len(ws)
    if ws:
        print('all  %7.2f %3.0f%% %7.2f %3.0f%% %7.2f %3.0f%% %7.2f %7.2f %3.0f%% %6.2f %6.2f %7.1f %7.1f %4d' % (
            tot['code'], pct(tot['code'], WT), tot['win'], pct(tot['win'], WT), tot['wout'],
            pct(tot['wout'], WT), tot['turn'], tot['ended'], pct(tot['ended'], WT), tot['rest'],
            tot['cpu'], mb(tot['nin']), mb(tot['nout']), tot['parts']))
        print('(%.2f of %d workers coding on average)' % (tot['code'] / T if T else 0, len(ws)))

    # the workers' waits for input, by what the main loop was doing meanwhile
    why = collections.Counter()
    for w in ws:
        why.update(W[w]['inwhy'])
    labs = sorted(why, key=lambda l: -why[l])

    def labname(l):
        if isinstance(l, tuple):
            return 'W%d ring full' % l[1]
        return {'ended': 'input ended', 'other': 'other wait', 'read': 'reading', 'write': 'writing',
                'working': 'working'}.get(l, str(l))
    if tot['win'] > 0:
        print('\nwaiting for input, by what the main loop did meanwhile (s; "W<k> ring full": its %s stopped'
              % ('parse' if dec else 'dealing'))
        print('by worker k\'s full input ring):')
        print('     ' + ''.join('%14s' % labname(l) for l in labs))
        for w in ws:
            print('W%-3d' % w + ''.join('%14.2f' % W[w]['inwhy'][l] for l in labs))

    # the main loop
    mt = collections.Counter()
    for a, b, lab in mspans:
        mt[lab] += b - a
    cpu = [e.dur for e in M if e.ev == 'cpu']
    busy = T - sum(mt.values())
    print('\nmain loop (s; %% of the run): working %.2f (%.0f%%)' % (busy, pct(busy, T)) + ''.join(
        ', %s %.2f (%.0f%%)' % (labname(l) if l not in ('read', 'write') else l, mt[l], pct(mt[l], T))
        for l in sorted(mt, key=lambda l: -mt[l])) + ('; cpu %.2f' % cpu[0] if cpu else ''))
    n_wait = sum(1 for e in M if e.ev == 'wait')
    print('  %d waits; "W<k> ring full": the %s stopped at a block for worker k, its input ring full%s'
          % (n_wait, 'parse' if dec else 'dealing',
             ' and dynbuff refused (the worker of the output busy)' if dec else ''))

    # how many coding at once
    pts = []
    for w in ws:
        for a, b, s in W[w]['spans']:
            if s == 'R':
                pts.append((a, 1))
                pts.append((b, -1))
    pts.sort()
    level = collections.Counter()
    k, last = 0, 0.0
    for t, d in pts:
        level[k] += t - last
        k, last = k + d, t
    level[k] += max(T - last, 0)
    if ws:
        print('\ncoding at once: ' + ', '.join('%d %.1f s (%.0f%%)' % (n, level[n], pct(level[n], T))
                                             for n in range(len(ws) + 1) if level[n] > 0))

    # the parts: each worker's rounds, each ended by its last output buffer (flags 2)
    if ws:
        print('\nparts (s): when the input came (its first and last buffer), when the output went (its first'
              ' and last), the waits in between')
        rows = []
        for w in ws:
            k, cur = 0, None
            for e in by_th['W%d' % w]:
                if cur is None:
                    cur = dict(i0=None, i1=None, o0=None, win=0.0, wout=0.0)
                if e.ev == 'pop' and e.n > 0:
                    if cur['i0'] is None:
                        cur['i0'] = e.t
                    cur['i1'] = e.t
                elif e.ev == 'push':
                    if cur['o0'] is None:
                        cur['o0'] = e.t
                    if e.x & 2:
                        rows.append((w + k * N, w, cur, e.t))
                        k, cur = k + 1, None
                elif e.ev == 'wait_in':
                    cur['win'] += e.dur
                elif e.ev == 'wait_out':
                    cur['wout'] += e.dur
        print(' part worker   input from      to  output from      to  wait in wait out')
        f = lambda v: '%7.2f' % v if v is not None else '      -'
        rows.sort(key=lambda r: r[0])
        for k, (part, w, c, t) in enumerate(rows):
            if len(rows) > 50 and 40 <= k < len(rows) - 10:
                if k == 40:
                    print('  ... (%d more)' % (len(rows) - 50))
                continue
            print('%5d   W%-3d    %s %s      %s %s %8.2f %8.2f' % (part, w, f(c['i0']), f(c['i1']), f(c['o0']),
                                                                  f(t), c['win'], c['wout']))

    # the timeline
    rows = []
    for w in ws:
        tl = Timeline(width, T)
        for a, b, s in W[w]['spans']:
            tl.add(a, b, s)
        if W[w]['exit'] is not None:
            tl.add(W[w]['exit'], T, ' ')
        rows.append(('W%d' % w, tl.chars('.')))
    tl = Timeline(width, T)
    for a, b, lab in mspans:
        s = wid(lab[1]) if isinstance(lab, tuple) else {'ended': 'E', 'other': '-', 'read': 'r',
                                                        'write': 'w'}[lab]
        tl.add(a, b, s)
    rows.append(('M', tl.chars('+')))
    bw = T / width if width else T
    rows.append(('j', ''.join(wid(j.at((k + 0.5) * bw)) for k in range(width))))
    n_at = Timeline(width, T)
    for t0, t1, _ in [(a, b, s) for w in ws for a, b, s in W[w]['spans'] if s == 'R']:
        n_at.add(t0, t1, 'R')
    rows.append(('n', ''.join(wid(min(int(c['R'] / bw + 0.5), len(IDS) - 1)) for c in n_at.col)))
    print('\ntimeline, %.2f s a column:' % bw)
    for name, line in rows:
        print('%-4s|%s|' % (name, line))
    print('  W<i>: R coding, i waiting for input, %s, . the rest, blank: ended' % (
        'O waiting for room for output while its output is written, o while another\'s is' if dec
        else 'o waiting for room for output'))
    print('  M: <k> stopped by worker k\'s full input ring, E the input ended, r reading, w writing,'
          ' - another wait, + working')
    print('  j: the worker whose %s; n: the workers coding' % (
        'output is written' if dec else 'turn it is to be dealt input'))

    # the container
    blk = [e for e in M if e.ev == 'blk']
    if blk:
        runs = []
        for e in blk:
            if runs and runs[-1][0] == e.w:
                runs[-1][1] += e.n
                runs[-1][2] += 1
            else:
                runs.append([e.w, e.n, 1])
        per = collections.defaultdict(list)
        for w, n, _ in runs:
            per[w].append(n)
        print('\ncontainer (%s): %d blocks, %.1f MB, in %d runs of one worker\'s blocks; an input ring holds '
              '%.1f MB' % ('as parsed' if dec else 'as written', len(blk), mb(sum(e.n for e in blk)), len(runs),
                           mb(ring)))
        say(' '.join('W%d:%.1f' % (w, mb(n)) for w, n, _ in runs[:40]) +
            (' ... (%d more)' % (len(runs) - 40) if len(runs) > 40 else ''))
        for w in sorted(per):
            v = per[w]
            print('  W%-3d %4d runs, %6.1f MB, the longest %6.1f MB, %3d longer than a ring' % (
                w, len(v), mb(sum(v)), mb(max(v)), sum(1 for n in v if n > ring)))

    # dynbuff
    if dec:
        held = collections.Counter()
        put = collections.Counter()
        top, top_t = 0.0, 0.0
        for e in M:
            if e.ev == 'blk' and e.x == 1:
                held[e.w] += e.n
                put[e.w] += e.n
            elif e.ev == 'dyn':
                held[e.w] -= e.n
            else:
                continue
            h = sum(held.values())
            if h > top:
                top, top_t = h, e.t
        mem = [e.n for e in M if e.ev == 'end']
        nb = sum(1 for e in M if e.ev == 'blk' and e.x == 1)
        print('\ndynbuff: %d blocks put aside, %.1f MB (%s); %.1f MB at most at once (at %.2f s); its memory '
              '%.1f MB' % (nb, mb(sum(put.values())), ', '.join('W%d %.1f' % (w, mb(put[w])) for w in sorted(put))
                           or 'none', mb(top), top_t, mb(mem[0]) if mem else 0))

    # so
    print('\nso:')
    if not ws:
        return
    say('the workers coded %.0f%% of their time (%.2f of %d on average)' % (
        pct(tot['code'], WT), tot['code'] / T if T else 0, len(ws)))
    stuck = sum(d for l, d in why.items() if isinstance(l, tuple))
    if tot['win'] > 0.05 * WT:
        say('%.0f%% waiting for input, %.0f%% of it while the %s stood at a block for a worker whose input ring was'
            ' full' % (pct(tot['win'], WT), pct(stuck, tot['win']), 'parse' if dec else 'dealing'))
        if dec and blk:
            say('(the container has %d runs of one worker\'s blocks, up to %.1f MB, and an input ring holds %.1f MB:'
                ' a run longer than its worker\'s ring holds the parse until that worker has decoded most of it,'
                ' and dynbuff takes blocks only while the worker of the output is idle)'
                % (len(runs), mb(max(n for _, n, _ in runs)), mb(ring)), 4)
    if tot['wout'] > 0.05 * WT:
        if dec:
            say('%.0f%% waiting for room for output, %.0f%% of it while other workers\' parts were written first'
                ' (the output goes in order)' % (pct(tot['wout'], WT), pct(tot['wout'] - tot['turn'], tot['wout'])))
        else:
            say('%.0f%% waiting for room for output' % pct(tot['wout'], WT))
    if tot['ended'] > 0.05 * WT:
        say('%.0f%% ended before the run (%d parts on %d workers: %s)' % (
            pct(tot['ended'], WT), tot['parts'], len(ws), ', '.join('W%d %d' % (w, W[w]['parts']) for w in ws)))
    if tot['cpu'] < 0.9 * tot['code']:
        say('the workers\' CPU time is %.0f%% of their coding time: they waited for a CPU' % pct(
            tot['cpu'], tot['code']))

def main():
    ap = argparse.ArgumentParser(description='where a run on threads spent its time (MTwrap_v3_dbg\'s dbg.txt)')
    ap.add_argument('file', nargs='?', default='dbg.txt')
    ap.add_argument('-w', '--width', type=int, default=64, help='the timeline\'s columns (64)')
    ap.add_argument('-r', '--run', type=int, help='this run alone')
    a = ap.parse_args()
    runs = load(a.file)
    if not runs:
        sys.exit('%s: no runs' % a.file)
    for r, (mode, ev) in runs.items():
        if a.run is None or a.run == r:
            analyse(r, mode, ev, max(a.width, 8))
            print()


if __name__ == '__main__':
    main()
