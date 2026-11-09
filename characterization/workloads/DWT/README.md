# DWT — Discrete Wavelet Transform

## Trace

- [Download compressed trace](../../../traces/dwt_8x_final.champsimtrace.xz)
- Dynamic instruction records: **64,254,895**
- Uncompressed record size: **64 bytes**
- Compressed file size: **4.76 MiB**
- Compressed SHA256: `11472d0224f633de78c7980f1e4bfefc990723c82d81894639e77c703d3a5dbf`
- Original raw-trace SHA256: `a70dd6fab2796ff708047bf360bcca007ff7757d543863d8f8d0410dea3c60fc`
- [Original characterization JSON](full_trace_original.json)

## Full-trace measurements

| Measurement | Result |
|---|---:|
| Load-address slot fraction | 77.986% |
| Branch-flagged instruction fraction | 8.219% |
| Consecutive loads sharing a 64-byte line | 66.523% |

## Four sampled execution windows

Each window contains 100,000 dynamic instruction records.
Windows begin near 0%, 25%, 50%, and 75% of the full trace.

| Measurement | Four-window summary |
|---|---:|
| Unique 64-byte lines in windows 1–4 | 1250/1250/1250/1249 |
| Largest/smallest sampled line-count ratio | 1.001x |
| Mean fraction of accesses to 10 hottest lines | 77.983% |
| Mean recent line reuse, original stream | 97.223% |
| Mean recent line reuse, 10 hottest lines excluded | 87.490% |
| Filtered minus original reuse | -9.734 percentage points |

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

