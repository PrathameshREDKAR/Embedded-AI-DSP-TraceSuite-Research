# IIR — Infinite Impulse Response Filtering

## Trace

- [Download compressed trace](../../../traces/iir_14x_final.champsimtrace.xz)
- Dynamic instruction records: **62,390,447**
- Uncompressed record size: **64 bytes**
- Compressed file size: **5.97 MiB**
- Compressed SHA256: `42df06b6304f18dbf581bd28146b21635711371e8ae5b8878406486d1d82aced`
- Original raw-trace SHA256: `aa538b825c52ed6e79ddf7034ae07ac8c25a083226803ef3cff4aa039c416bc1`
- [Original characterization JSON](full_trace_original.json)

## Full-trace measurements

| Measurement | Result |
|---|---:|
| Load-address slot fraction | 66.667% |
| Branch-flagged instruction fraction | 11.765% |
| Consecutive loads sharing a 64-byte line | 37.500% |

## Four sampled execution windows

Each window contains 100,000 dynamic instruction records.
Windows begin near 0%, 25%, 50%, and 75% of the full trace.

| Measurement | Four-window summary |
|---|---:|
| Unique 64-byte lines in windows 1–4 | 1475/1474/1474/1474 |
| Largest/smallest sampled line-count ratio | 1.001x |
| Mean fraction of accesses to 10 hottest lines | 83.424% |
| Mean recent line reuse, original stream | 97.911% |
| Mean recent line reuse, 10 hottest lines excluded | 87.486% |
| Filtered minus original reuse | -10.426 percentage points |

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

