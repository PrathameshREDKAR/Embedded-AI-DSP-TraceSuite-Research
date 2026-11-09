# STFT — Short-Time Fourier Transform

## Trace

- [Download compressed trace](../../../traces/stft_3x_final.champsimtrace.xz)
- Dynamic instruction records: **80,714,506**
- Uncompressed record size: **64 bytes**
- Compressed file size: **8.35 MiB**
- Compressed SHA256: `ec5ecc2ab3f71f5e8ef34c94d28fd49f686d0598a406a09bd225cb606959f324`
- Original raw-trace SHA256: `6121774d556d8d80192ece3033927b7751d971fd60a4b3a02b273e055d2b7aee`
- [Original characterization JSON](full_trace_original.json)

## Full-trace measurements

| Measurement | Result |
|---|---:|
| Load-address slot fraction | 63.844% |
| Branch-flagged instruction fraction | 5.754% |
| Consecutive loads sharing a 64-byte line | 59.431% |

## Four sampled execution windows

Each window contains 100,000 dynamic instruction records.
Windows begin near 0%, 25%, 50%, and 75% of the full trace.

| Measurement | Four-window summary |
|---|---:|
| Unique 64-byte lines in windows 1–4 | 156/86/90/169 |
| Largest/smallest sampled line-count ratio | 1.965x |
| Mean fraction of accesses to 10 hottest lines | 75.714% |
| Mean recent line reuse, original stream | 99.435% |
| Mean recent line reuse, 10 hottest lines excluded | 97.969% |
| Filtered minus original reuse | -1.466 percentage points |

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

