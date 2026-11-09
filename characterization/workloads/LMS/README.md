# LMS — Least Mean Squares

## Trace

- [Download compressed trace](../../../traces/lms_3x_final.champsimtrace.xz)
- Dynamic instruction records: **64,722,031**
- Uncompressed record size: **64 bytes**
- Compressed file size: **4.03 MiB**
- Compressed SHA256: `ff73d863df9a69419defedc4847f5c6d77fb0ec12a35d4ae6d46ea9bc306789e`
- Original raw-trace SHA256: `e179105e2d228946467ba0d1af9f43f778ace30b1a40dc160c8c5cdb70e45ddc`
- [Original characterization JSON](full_trace_original.json)

## Full-trace measurements

| Measurement | Result |
|---|---:|
| Load-address slot fraction | 81.157% |
| Branch-flagged instruction fraction | 10.817% |
| Consecutive loads sharing a 64-byte line | 38.938% |

## Four sampled execution windows

Each window contains 100,000 dynamic instruction records.
Windows begin near 0%, 25%, 50%, and 75% of the full trace.

| Measurement | Four-window summary |
|---|---:|
| Unique 64-byte lines in windows 1–4 | 34/32/32/34 |
| Largest/smallest sampled line-count ratio | 1.062x |
| Mean fraction of accesses to 10 hottest lines | 88.077% |
| Mean recent line reuse, original stream | 99.734% |
| Mean recent line reuse, 10 hottest lines excluded | 99.647% |
| Filtered minus original reuse | -0.087 percentage points |

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

