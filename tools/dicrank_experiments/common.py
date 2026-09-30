"""Shared code for the dict.rank experiments (docs/english_dic_reconstruction.md).

Paths come from the environment:
  ENWIK8       enwik8                    (default data/enwik8)
  ENWIK9       enwik9                    (default data/enwik9)
  ENGLISH_DIC  fx2-cmix dictionary       (default english.dic)
  WORK         cache and output folder   (default work)
Run prep.py and prep9.py first; they cache the tokenized texts in WORK.
Needs numpy and scipy.
"""
import math
import os
import pickle
import re

import numpy as np
import scipy.sparse as sp
import scipy.sparse.linalg as sla

ENWIK8 = os.environ.get('ENWIK8', 'data/enwik8')
ENWIK9 = os.environ.get('ENWIK9', 'data/enwik9')
ENGLISH_DIC = os.environ.get('ENGLISH_DIC', 'english.dic')
WORK = os.environ.get('WORK', 'work')
os.makedirs(WORK, exist_ok=True)

# cmix's word transform split: 2+ capitals form a word, a capital can start a
# lowercase run, lowercase runs; words are lowercased afterwards.
TOKEN = re.compile(rb'[A-Z]{2,}|[A-Z][a-z]*|[a-z]+')
HEAD = 3920  # explicitly ordered words (1- and 2-byte codeword tiers)


def work(name):
    return os.path.join(WORK, name)


def load():
    """Candidate words (dicrank order), enwik8 token ids and the dictionary structure."""
    words, counts = pickle.load(open(work('candidates.pkl'), 'rb'))
    rank = {w: i for i, w in enumerate(words)}
    dic = open(ENGLISH_DIC, 'rb').read().split(b'\n')[:-1]
    head, tail = dic[:HEAD], dic[HEAD:]
    run_of, r = {}, 0
    for i, w in enumerate(tail):
        if i and w <= tail[i - 1]:
            r += 1
        run_of[w] = r
    return dict(words=words, counts=np.array(counts), rank=rank, dic=dic, head=head,
                tail=tail, run_of=run_of, runs=r + 1, ids=np.load(work('ids8.npy')))


def ppmi(token_ids, targets, n_words, K=4000):
    """Sparse PPMI matrix: rows = target words (candidate ranks), columns = the
    left and right neighbour among the K most frequent words. token_ids may hold
    n_words as a sentinel for words outside the candidate list."""
    tidx = np.full(n_words + 1, -1, np.int64)
    tidx[np.asarray(targets)] = np.arange(len(targets))
    left, mid, right = token_ids[:-2], token_ids[1:-1], token_ids[2:]
    t = tidx[mid]
    m1 = (t >= 0) & (left < K)
    m2 = (t >= 0) & (right < K)
    rows = np.concatenate([t[m1], t[m2]])
    cols = np.concatenate([left[m1], right[m2] + K])
    M = sp.coo_matrix((np.ones(len(rows)), (rows, cols)), shape=(len(targets), 2 * K)).tocsr()
    M.sum_duplicates()
    tot = M.sum()
    rs = np.asarray(M.sum(1)).ravel() + 1e-9
    cs = np.asarray(M.sum(0)).ravel() + 1e-9
    C = M.tocoo()
    pmi = np.log(C.data * tot / (rs[C.row] * cs[C.col]))
    keep = pmi > 0
    return sp.coo_matrix((pmi[keep], (C.row[keep], C.col[keep])), shape=M.shape).tocsr()


def unit_rows(X):
    if sp.issparse(X):
        n = np.sqrt(np.asarray(X.multiply(X).sum(1)).ravel()) + 1e-12
        return (sp.diags(1 / n) @ X).tocsr()
    return X / (np.linalg.norm(X, axis=1, keepdims=True) + 1e-12)


def svds(X, k):
    """Truncated SVD with a fixed start vector, so results repeat run to run."""
    n = min(X.shape)
    return sla.svds(X, k=k, v0=np.full(n, 1 / math.sqrt(n)))


def svd_vectors(P, dim=200):
    u, s, _ = svds(P, dim)
    return unit_rows(u * np.sqrt(s))


def add_suffix_features(V, target_words, weight=0.5, dim=64):
    """Append SVD-reduced one-hot features of the last 1-3 letters."""
    feats, rows, cols = {}, [], []
    for i, w in enumerate(target_words):
        for k in (1, 2, 3):
            if len(w) > k:
                rows.append(i)
                cols.append(feats.setdefault((k, w[-k:]), len(feats)))
    X = sp.coo_matrix((np.ones(len(rows)), (rows, cols)), shape=(len(target_words), len(feats))).tocsr()
    us, ss, _ = svds(X, dim)
    return unit_rows(np.hstack([unit_rows(V), weight * unit_rows(us * np.sqrt(ss))]))


def adaptive_cost(symbols, alphabet, alpha=0.4):
    """Bytes for an adaptive order-0 frequency coder over `alphabet` symbols."""
    cnt, tot, bits = {}, 0, 0.0
    for s in symbols:
        n = cnt.get(s, 0)
        bits -= math.log2((n + alpha) / (tot + alpha * alphabet))
        cnt[s] = n + 1
        tot += 1
    return bits / 8


def run_predictions(V, truth, runs, beta=None):
    """Visit the words in the given order; score the runs started so far by the
    cosine between the word vector and each run's centroid (built from words
    already placed). beta=None: return rank symbols (-1 = new run) and the bits
    for naming new runs. beta=number: return the bytes of a softmax model
    p(run) ~ (n_run + 0.5) * exp(beta * cos) with an adaptive new-run escape."""
    dense = not sp.issparse(V)
    d = V.shape[1]
    C = np.zeros((runs, d))
    Cn = np.zeros((runs, d))
    n = np.zeros(runs)
    started = np.zeros(runs, bool)
    syms, bits, new_seen, unstarted = [], 0.0, 0, runs
    for j, r in enumerate(truth):
        v = V[j] if dense else np.asarray(V[j].todense()).ravel()
        if started[r]:
            s = Cn @ v
            if beta is None:
                s[~started] = -np.inf
                syms.append(int((s > s[r]).sum()))
            else:
                p_new = (new_seen + 0.5) / (j + 1.0)
                lg = np.log(n + 0.5) + beta * s
                lg[~started] = -np.inf
                p = np.exp(lg - lg.max())
                p /= p.sum()
                bits -= math.log2((1 - p_new) * p[r])
        else:
            if beta is None:
                syms.append(-1)
            else:
                bits -= math.log2((new_seen + 0.5) / (j + 1.0))
            bits += math.log2(unstarted)
            new_seen += 1
            unstarted -= 1
            started[r] = True
        C[r] += v
        Cn[r] = C[r] / np.linalg.norm(C[r])
        n[r] += 1
    return (syms, bits / 8) if beta is None else bits / 8


def head_order(V, beta=10.0, prev2=0.5):
    """Head words in dictionary order: each next word is scored among the
    remaining head words by similarity to the previous word (+ prev2 x the one
    before). Returns (softmax bytes, rank symbols)."""
    n = V.shape[0]
    remaining = np.ones(n, bool)
    remaining[0] = False
    bits, syms = math.log2(n), []
    for i in range(1, n):
        q = V[i - 1] + (prev2 * V[i - 2] if i >= 2 else 0)
        s = V @ q
        s[~remaining] = -np.inf
        syms.append(int((s > s[i]).sum()))
        p = np.exp(beta * (s - s.max()))
        p /= p.sum()
        bits -= math.log2(p[i])
        remaining[i] = False
    return bits / 8, syms
