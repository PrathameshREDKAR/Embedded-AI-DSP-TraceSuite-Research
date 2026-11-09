# MATMUL — Matrix Multiplication

## Trace

- [Download compressed trace](../../../traces/matmul_2x_final.champsimtrace.xz)
- Dynamic instruction records: **101,321,253**
- Uncompressed record size: **64 bytes**
- Compressed file size: **5.67 MiB**
- Compressed SHA256: `4a8e068d942ecebd7f7a118ec9c4cf1b3b9af30c63d5d4679d5a5a52fd4504ae`
- Original raw-trace SHA256: `e833bb132fad3b4642aaf9ae0b577ff81a10c3cfe1e1936125e7b72fa8072c09`
- [Original characterization JSON](full_trace_original.json)

## Full-trace measurements

| Measurement | Result |
|---|---:|
| Load-address slot fraction | 81.664% |
| Branch-flagged instruction fraction | 8.410% |
| Consecutive loads sharing a 64-byte line | 55.614% |

## Four sampled execution windows

Each window contains 100,000 dynamic instruction records.
Windows begin near 0%, 25%, 50%, and 75% of the full trace.

| Measurement | Four-window summary |
|---|---:|
| Unique 64-byte lines in windows 1–4 | 665/664/665/664 |
| Largest/smallest sampled line-count ratio | 1.002x |
| Mean fraction of accesses to 10 hottest lines | 86.428% |
| Mean recent line reuse, original stream | 89.700% |
| Mean recent line reuse, 10 hottest lines excluded | 89.485% |
| Filtered minus original reuse | -0.215 percentage points |

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

