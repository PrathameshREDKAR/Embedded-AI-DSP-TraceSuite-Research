# FIR — Finite Impulse Response Filtering

## Trace

- [Download compressed trace](../../../traces/fir_13x_final.champsimtrace.xz)
- Dynamic instruction records: **68,105,642**
- Uncompressed record size: **64 bytes**
- Compressed file size: **4.41 MiB**
- Compressed SHA256: `fca9d0552d3fe782365a73b7ff3597c5099bd9f0b82f560da542c8d24e2d183e`
- Original raw-trace SHA256: `0d90a089863922c3fe4e33b3677e9c718755f30158646d82685f1508ccefae91`
- [Original characterization JSON](full_trace_original.json)

## Full-trace measurements

| Measurement | Result |
|---|---:|
| Load-address slot fraction | 81.424% |
| Branch-flagged instruction fraction | 10.162% |
| Consecutive loads sharing a 64-byte line | 55.746% |

## Four sampled execution windows

Each window contains 100,000 dynamic instruction records.
Windows begin near 0%, 25%, 50%, and 75% of the full trace.

| Measurement | Four-window summary |
|---|---:|
| Unique 64-byte lines in windows 1–4 | 39/41/40/41 |
| Largest/smallest sampled line-count ratio | 1.051x |
| Mean fraction of accesses to 10 hottest lines | 90.734% |
| Mean recent line reuse, original stream | 99.927% |
| Mean recent line reuse, 10 hottest lines excluded | 99.408% |
| Filtered minus original reuse | -0.519 percentage points |

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

