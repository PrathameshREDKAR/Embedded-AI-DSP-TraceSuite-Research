# CONV — Convolution

## Trace

- [Download compressed trace](../../../traces/convolution_3x_final.champsimtrace.xz)
- Dynamic instruction records: **65,426,935**
- Uncompressed record size: **64 bytes**
- Compressed file size: **3.66 MiB**
- Compressed SHA256: `bd3f251988410a9244b44e2bd91c0c1aa051670963905b844d4d89b802344455`
- Original raw-trace SHA256: `cdc45bc1d5fc8dc927386f886ae884728863c9f4e00c4908501cd940596bde82`
- [Original characterization JSON](full_trace_original.json)

## Full-trace measurements

| Measurement | Result |
|---|---:|
| Load-address slot fraction | 81.576% |
| Branch-flagged instruction fraction | 10.063% |
| Consecutive loads sharing a 64-byte line | 53.230% |

## Four sampled execution windows

Each window contains 100,000 dynamic instruction records.
Windows begin near 0%, 25%, 50%, and 75% of the full trace.

| Measurement | Four-window summary |
|---|---:|
| Unique 64-byte lines in windows 1–4 | 145/144/144/144 |
| Largest/smallest sampled line-count ratio | 1.007x |
| Mean fraction of accesses to 10 hottest lines | 94.061% |
| Mean recent line reuse, original stream | 99.653% |
| Mean recent line reuse, 10 hottest lines excluded | 94.748% |
| Filtered minus original reuse | -4.904 percentage points |

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

