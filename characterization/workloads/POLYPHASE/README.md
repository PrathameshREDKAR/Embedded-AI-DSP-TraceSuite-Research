# POLYPHASE — Polyphase Filtering

## Trace

- [Download compressed trace](../../../traces/polyphase_4x_final.champsimtrace.xz)
- Dynamic instruction records: **69,065,955**
- Uncompressed record size: **64 bytes**
- Compressed file size: **3.81 MiB**
- Compressed SHA256: `813a3c2da75ad7c163800e701675258d8161aee89ce49211f317c1b3b43d0ceb`
- Original raw-trace SHA256: `0bcadb48ebda45badf8f84ac25be5e8b0176b46acde9ca0118eac6f42f903c77`
- [Original characterization JSON](full_trace_original.json)

## Full-trace measurements

| Measurement | Result |
|---|---:|
| Load-address slot fraction | 76.841% |
| Branch-flagged instruction fraction | 15.531% |
| Consecutive loads sharing a 64-byte line | 64.579% |

## Four sampled execution windows

Each window contains 100,000 dynamic instruction records.
Windows begin near 0%, 25%, 50%, and 75% of the full trace.

| Measurement | Four-window summary |
|---|---:|
| Unique 64-byte lines in windows 1–4 | 43/43/43/42 |
| Largest/smallest sampled line-count ratio | 1.024x |
| Mean fraction of accesses to 10 hottest lines | 94.638% |
| Mean recent line reuse, original stream | 99.922% |
| Mean recent line reuse, 10 hottest lines excluded | 98.907% |
| Filtered minus original reuse | -1.015 percentage points |

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

