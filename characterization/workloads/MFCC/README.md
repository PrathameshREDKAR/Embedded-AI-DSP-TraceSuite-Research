# MFCC — Mel-Frequency Cepstral Coefficients

## Trace

- [Download compressed trace](../../../traces/mfcc_2x_final.champsimtrace.xz)
- Dynamic instruction records: **114,614,807**
- Uncompressed record size: **64 bytes**
- Compressed file size: **11.86 MiB**
- Compressed SHA256: `19d00ae69fe54faba34fb3406b89412ded49e65adc57d74e4100f45fa5909c56`
- Original raw-trace SHA256: `3cdee02ad72dc62792b26a2237651bbe00e7873f79781627f984bb1f1a037acb`
- [Original characterization JSON](full_trace_original.json)

## Full-trace measurements

| Measurement | Result |
|---|---:|
| Load-address slot fraction | 67.897% |
| Branch-flagged instruction fraction | 3.209% |
| Consecutive loads sharing a 64-byte line | 55.124% |

## Four sampled execution windows

Each window contains 100,000 dynamic instruction records.
Windows begin near 0%, 25%, 50%, and 75% of the full trace.

| Measurement | Four-window summary |
|---|---:|
| Unique 64-byte lines in windows 1–4 | 1257/144/1257/144 |
| Largest/smallest sampled line-count ratio | 8.729x |
| Mean fraction of accesses to 10 hottest lines | 80.838% |
| Mean recent line reuse, original stream | 98.838% |
| Mean recent line reuse, 10 hottest lines excluded | 94.617% |
| Filtered minus original reuse | -4.221 percentage points |

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

