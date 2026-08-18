def pat(v,w):
    assert 0<=v<(1<<w), (v,w)
    return format(v,'0%db'%w)

L=[]
A=L.append
A("# bmg tunable parameters -- see IDX-FORMAT.md")
A("# Rate1 X, base!pattern   ->  X = eSCALE/(pattern+base)     (counter EMA rate)")
A("# Number X, mult, base!pattern -> X = (pattern+base)*mult    (plain integer)")
A("")
A("Prefix P0")
A("Debug 1")
A("Const 0")
A("")

def rate1(name,val,base=16,w=13,c=""):
    # wr = eSCALE/(v+base) ; caller passes desired divisor d = v+base
    v=val-base
    A("# wr = %d %s"%(524288//val, c)); A("Rate1  %-10s %2d!%s"%(name+",",base,pat(v,w)))
def number(name,val,mult=1,base=1,w=12,c=""):
    v=val//mult-base
    A("# %s = %d %s"%(name,val,c)); A("Number %-10s %d, %d!%s"%(name+",",mult,base,pat(v,w)))

A("# ---- counter learning rates and probability floors ----------------------")
rate1("CT_wr",  384)      # slow context counters   wr=1365 (~1/24)
number("CT_mw",  16)
rate1("CTF_wr", 160)      # fast context counters   wr=3276 (~1/10)
number("CTF_mw", 40)
rate1("CTN_wr",1280)      # neighbour (off-context) update rate
rate1("EL_wr",  384)      # escape level counters
number("EL_mw",  24)
rate1("EP_wr",  384)      # escape payload counters
number("EP_mw",  24)
rate1("BY_wr",  320)      # generic byte model
number("BY_mw",  20)
rate1("PR_wr",  256)      # prologue / palette model
number("PR_mw",  16)
rate1("RL_wr",  256)      # RLE structure model
number("RL_mw",  24)
rate1("FL_wr",  512)      # one-off flags
number("FL_mw", 400)
A("")
A("# ---- mixer and SSE -------------------------------------------------------")
number("MX_LR",  12, 1, 1, 8)      # mixer learning rate numerator
number("MX_W0", 11000, 1, 1, 16)   # initial mixer weight (1<<16 == 1.0)
number("AP_RATE", 7, 1, 1, 5)      # SSE adaptation shift
number("AP_WT",   7, 1, 1, 5)      # first SSE share of the final probability, /16
number("AP_WT2",  6, 1, 1, 5)      # second SSE share, /16
A("")
A("# ---- activity quantiser --------------------------------------------------")
A("# eight activity levels; the ladder is sorted and clamped at the point of use")
for i,v in enumerate([1,2,4,8,14,35,103]):
    number("ACT_E%d"%i, v, 1, 1, 9)
A("# eight escape-strip groups, deliberately a different ladder")
for i,v in enumerate([1,3,6,10,16,27,52]):
    number("GRP_E%d"%i, v, 1, 1, 9)
A("# activity weights: W, N, NW, NE, NN, WW, and the cross-plane term")
for n,v in [("AW_W",6),("AW_N",4),("AW_NW",3),("AW_NE",3),("AW_NN",2),("AW_WW",2),("AW_X",4)]:
    number(n, v, 1, 0, 5)
A("# per-level deadband for the ternary features")
for i,v in enumerate([1,1,2,2,2,4,4,4]):
    number("DZ%d"%i, v, 1, 1, 5)
A("")
A("# ---- predictor -----------------------------------------------------------")
A("# (the NLMS filter and the bias corrector that used to live here were")
A("#  measured and removed -- see BMG-FORMAT.md)")
open("IDX/bmg-P0.idx","w").write("\n".join(L)+"\n")
print("\n".join(L))
