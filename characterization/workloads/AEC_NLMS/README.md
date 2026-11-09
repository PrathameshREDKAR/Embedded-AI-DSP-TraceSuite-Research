# AEC_NLMS — Acoustic Echo Cancellation / Normalized LMS

## Trace

- [Download compressed trace](../../../traces/aec_nlms_1x_accepted_pilot.champsimtrace.xz)
- Dynamic instruction records: **104,556,305**
- Uncompressed record size: **64 bytes**
- Compressed file size: **11.71 MiB**
- Compressed SHA256: `92791ee4c4ffeb357f8302ecd8143cc79e42c73c4bf6d98ff6563d045c5330b7`
- Original raw-trace SHA256: `e79a000aeb53fc22bbe74d9023544b2cac22ff1a3219a902d90cab506a4b1e3c`
- [Original characterization JSON](full_trace_original.json)

## Full-trace measurements

| Measurement | Result |
|---|---:|
| Load-address slot fraction | 77.704% |
| Branch-flagged instruction fraction | 8.086% |
| Consecutive loads sharing a 64-byte line | 52.314% |

## Four sampled execution windows

Each window contains 100,000 dynamic instruction records.
Windows begin near 0%, 25%, 50%, and 75% of the full trace.

| Measurement | Four-window summary |
|---|---:|
| Unique 64-byte lines in windows 1–4 | 45/42/43/43 |
| Largest/smallest sampled line-count ratio | 1.071x |
| Mean fraction of accesses to 10 hottest lines | 86.933% |
| Mean recent line reuse, original stream | 97.962% |
| Mean recent line reuse, 10 hottest lines excluded | 99.530% |
| Filtered minus original reuse | +1.568 percentage points |

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

**Provenance exception:** The original JSON says `COMPLETE_PROVISIONAL` and `acceptance_verified=false`. The trace originated as a 1x Pin sizing pilot and was subsequently accepted as the canonical final trace by separate frozen closure and trace-selection attestations. Those attestation documents must be linked before public release. The original JSON is preserved unchanged.

