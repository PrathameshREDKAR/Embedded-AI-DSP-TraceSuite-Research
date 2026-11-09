# FFT — Fast Fourier Transform

## Trace

- [Download compressed trace](../../../traces/fft_100x_final.champsimtrace.xz)
- Dynamic instruction records: **69,607,705**
- Uncompressed record size: **64 bytes**
- Compressed file size: **5.87 MiB**
- Compressed SHA256: `d9b9d9a37f18b39f28b6818391afb2a152abace68aeb65396902771500dcbc89`
- Original raw-trace SHA256: `19d1fb8678e87db07c1e2fb37658a3bbf58175001da32fd1c59422cf6dc9f791`
- [Original characterization JSON](full_trace_original.json)

## Full-trace measurements

| Measurement | Result |
|---|---:|
| Load-address slot fraction | 74.999% |
| Branch-flagged instruction fraction | 6.038% |
| Consecutive loads sharing a 64-byte line | 70.718% |

## Four sampled execution windows

Each window contains 100,000 dynamic instruction records.
Windows begin near 0%, 25%, 50%, and 75% of the full trace.

| Measurement | Four-window summary |
|---|---:|
| Unique 64-byte lines in windows 1–4 | 263/263/263/263 |
| Largest/smallest sampled line-count ratio | 1.000x |
| Mean fraction of accesses to 10 hottest lines | 95.270% |
| Mean recent line reuse, original stream | 99.166% |
| Mean recent line reuse, 10 hottest lines excluded | 85.027% |
| Filtered minus original reuse | -14.139 percentage points |

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

