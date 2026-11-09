# DCT — Discrete Cosine Transform

## Trace

- [Download compressed trace](../../../traces/dct_2x_final.champsimtrace.xz)
- Dynamic instruction records: **63,009,469**
- Uncompressed record size: **64 bytes**
- Compressed file size: **2.52 MiB**
- Compressed SHA256: `531175ed9a375a3a98e7c93f8e522b79b31a347360e1c70324f893a43b3f8809`
- Original raw-trace SHA256: `c056e9e97d5566ec238f59aefc50b813fb9c76f031ebfed9562a9605279cfa12`
- [Original characterization JSON](full_trace_original.json)

## Full-trace measurements

| Measurement | Result |
|---|---:|
| Load-address slot fraction | 81.172% |
| Branch-flagged instruction fraction | 8.549% |
| Consecutive loads sharing a 64-byte line | 29.336% |

## Four sampled execution windows

Each window contains 100,000 dynamic instruction records.
Windows begin near 0%, 25%, 50%, and 75% of the full trace.

| Measurement | Four-window summary |
|---|---:|
| Unique 64-byte lines in windows 1–4 | 89/87/89/87 |
| Largest/smallest sampled line-count ratio | 1.023x |
| Mean fraction of accesses to 10 hottest lines | 91.697% |
| Mean recent line reuse, original stream | 99.550% |
| Mean recent line reuse, 10 hottest lines excluded | 96.918% |
| Filtered minus original reuse | -2.632 percentage points |

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

