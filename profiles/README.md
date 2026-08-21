# profiles

Verified parameter sets for `wavs3`, in the `mdesc()` form `import.sh` and
`apply_export.pl` both read.  Every file here has been through the whole
source path -- `import.sh` into a copy of `IDX/`, `mk.sh` rebuild, encode,
decode -- and reproduced its recorded size with a byte-identical round trip.
A number measured only by patching a binary's `!MAP!` strings is not recorded
here, because that is not the same claim.

They live in git because the optimizer's own state does not: `opt/<inst>/` and
the merge scratch are gitignored and were lost to four container reclaims over
the course of the run.  The exports are 5 KB each, so keeping the ones that
matter costs nothing and makes the results survive the workspace.

| file | wavs3 | vs 2022001 baseline | origin |
|------|-------|--------------------|--------|
| `best-2020023.exp`   | 2020023 | -1978 | instance b, after it walked the `G0_MATCH_*` / `G0_MLR_` group |
| `hybrid-2020503.exp` | 2020503 | -1498 | `merge_opt.pl` round 14, recombining all four instances |

`best-2020023.exp` is the better set and the one to fold into `IDX/` if the
tree is to keep a single result.  It is worth keeping the hybrid alongside it
anyway: the two were produced by different mechanisms and disagree on a
number of descriptors, so the hybrid is useful material for a later
`merge_opt.pl` run even though it is no longer the leader.

To apply one:

    ./import.sh profiles/best-2020023.exp IDX      # fold into the sources
    ./snapshot.sh --export=profiles/best-2020023.exp out   # or just measure it
