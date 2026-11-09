# BEAMFORM — Delay-and-Sum Beamforming

## Trace

- [Download compressed trace](../../../traces/beamforming_1x_final.champsimtrace.xz)
- Dynamic instruction records: **346,893,926**
- Uncompressed record size: **64 bytes**
- Compressed file size: **16.76 MiB**
- Compressed SHA256: `154794b77f2db8c3f134441d94e4cbec7578c6ffa72841dcddf3f4f8ee4890a2`
- Original raw-trace SHA256: `19403b602dbad9f3b8c3bd626ab2286fdf199878eb707b51a21824072e90fe28`
- [Original characterization JSON](full_trace_original.json)

## Full-trace measurements

| Measurement | Result |
|---|---:|
| Load-address slot fraction | 67.256% |
| Branch-flagged instruction fraction | 18.156% |
| Consecutive loads sharing a 64-byte line | 48.684% |

## Four sampled execution windows

Each window contains 100,000 dynamic instruction records.
Windows begin near 0%, 25%, 50%, and 75% of the full trace.

| Measurement | Four-window summary |
|---|---:|
| Unique 64-byte lines in windows 1–4 | 181/183/180/182 |
| Largest/smallest sampled line-count ratio | 1.017x |
| Mean fraction of accesses to 10 hottest lines | 96.270% |
| Mean recent line reuse, original stream | 99.726% |
| Mean recent line reuse, 10 hottest lines excluded | 93.052% |
| Filtered minus original reuse | -6.673 percentage points |

## Interpretation

Load-address fraction counts nonzero recorded memory-address slots,
not the fraction of instructions that perform loads.
Recent reuse means a 64-byte address line appeared among the
previous 1,024 memory-address slots in the respective sampled stream.
The filtered result excludes the ten most frequently referenced
lines *within each window* and recomputes history on the remaining stream.

These measurements are **not cache hit rates, miss rates,
prefetcher accuracy, or measured speedups**.
Four windows from one trace are descriptive observations,
not independent experimental trials.

See the [complete characterization overview](../../README.md)
and [window-level CSV data](../../data/temporal_windows.csv).

