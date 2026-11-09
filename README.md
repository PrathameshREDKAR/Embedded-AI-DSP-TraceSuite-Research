# Embedded AI/DSP Prefetching Trace Suite

**A research artifact for studying memory-access behavior and hardware prefetching across embedded and digital signal processing workloads.**

The project provides accepted instruction traces, workload implementations, trace-generation tools, characterization datasets, scientific findings, and experimental evidence for Athena/ChampSim-style trace-driven research.

## At a glance

| Research artifact | Available |
|---|---:|
| Accepted application traces | 13 |
| Dynamic instruction records characterized | 1,274,683,876 |
| Workload C++ programs | 13 |
| Individual characterization reports | 13 |
| Sampled temporal-analysis windows | 52 |
| Hot-line sensitivity windows | 52 |
| Original Python characterization scripts | 7 |
| Special Intel Pin compatibility tracer variants | 2 |

The broader workload-generation project targets 20 workloads. **Thirteen have been completed and accepted; the remaining seven are not represented as finished datasets.**

## Why this project?

Hardware prefetchers attempt to predict future memory accesses and fetch data before the processor needs it.

Different applications generate different memory-access patterns. A prefetcher effective for one workload may perform differently on another.

This project studies these differences using instruction-level traces from signal processing, adaptive filtering, audio processing, matrix computation, and beamforming.

The objective is to provide inspectable workload artifacts and measurements that support research into memory locality, workload variation, and hardware prefetcher evaluation.

## Workloads and characterization

Each workload has a compressed trace, corresponding C++ source program, and a dedicated characterization page.

The same-line column reports the percentage of consecutive recorded load-address slots that map to the same 64-byte line. The window ratio compares the largest and smallest unique-line counts across four sampled 100,000-instruction windows.

| Workload | Instruction records | Same-line loads | Window ratio | Trace | Characterization |
|---|---:|---:|---:|---|---|
| FFT | 69,607,705 | 70.72% | 1.000x | [XZ](traces/fft_100x_final.champsimtrace.xz) | [Findings](characterization/workloads/FFT/README.md) |
| FIR | 68,105,642 | 55.75% | 1.051x | [XZ](traces/fir_13x_final.champsimtrace.xz) | [Findings](characterization/workloads/FIR/README.md) |
| Convolution | 65,426,935 | 53.23% | 1.007x | [XZ](traces/convolution_3x_final.champsimtrace.xz) | [Findings](characterization/workloads/CONV/README.md) |
| Matrix multiplication | 101,321,253 | 55.61% | 1.002x | [XZ](traces/matmul_2x_final.champsimtrace.xz) | [Findings](characterization/workloads/MATMUL/README.md) |
| IIR | 62,390,447 | 37.50% | 1.001x | [XZ](traces/iir_14x_final.champsimtrace.xz) | [Findings](characterization/workloads/IIR/README.md) |
| DCT | 63,009,469 | 29.34% | 1.023x | [XZ](traces/dct_2x_final.champsimtrace.xz) | [Findings](characterization/workloads/DCT/README.md) |
| DWT | 64,254,895 | 66.52% | 1.001x | [XZ](traces/dwt_8x_final.champsimtrace.xz) | [Findings](characterization/workloads/DWT/README.md) |
| LMS | 64,722,031 | 38.94% | 1.062x | [XZ](traces/lms_3x_final.champsimtrace.xz) | [Findings](characterization/workloads/LMS/README.md) |
| Polyphase filtering | 69,065,955 | 64.58% | 1.024x | [XZ](traces/polyphase_4x_final.champsimtrace.xz) | [Findings](characterization/workloads/POLYPHASE/README.md) |
| STFT | 80,714,506 | 59.43% | 1.965x | [XZ](traces/stft_3x_final.champsimtrace.xz) | [Findings](characterization/workloads/STFT/README.md) |
| MFCC | 114,614,807 | 55.12% | 8.729x | [XZ](traces/mfcc_2x_final.champsimtrace.xz) | [Findings](characterization/workloads/MFCC/README.md) |
| Beamforming | 346,893,926 | 48.68% | 1.017x | [XZ](traces/beamforming_1x_final.champsimtrace.xz) | [Findings](characterization/workloads/BEAMFORM/README.md) |
| AEC/NLMS | 104,556,305 | 52.31% | 1.071x | [XZ](traces/aec_nlms_1x_accepted_pilot.champsimtrace.xz) | [Findings](characterization/workloads/AEC_NLMS/README.md) |

**Total: 1,274,683,876 dynamic instruction records.**

See the [complete characterization overview](characterization/README.md) for definitions, datasets, evidence, and limitations.

## Main characterization findings

### 1. Memory-reference locality differs across workloads

The consecutive-load same-64-byte-line fraction ranges from approximately **29.34% for DCT** to **70.72% for FFT**.

This indicates differences in recorded load-address locality across the workloads. It is not a measured cache-hit rate.

### 2. Some workloads show substantial window-to-window variation

In the four sampled MFCC windows, the number of distinct 64-byte lines alternated between **1,257 and 144**, a largest-to-smallest ratio of approximately **8.73x**.

STFT also showed variation, with unique-line counts of 156, 86, 90, and 169.

These are observed changes across sampled portions of execution, not verified algorithmic phase boundaries.

### 3. Frequently accessed address lines strongly influence reuse measurements

Removing the ten most frequently referenced cache lines in each sampled window changed measured recent reuse.

Examples of average filtered-minus-original reuse include:

- FFT: -14.139 percentage points
- IIR: -10.426 percentage points
- DWT: -9.734 percentage points
- AEC/NLMS: +1.568 percentage points

The filtered experiment recomputes address history after excluding those references. It changes the input stream and must not be interpreted as physical cache eviction.

### 4. Measurements should not be confused with performance results

The characterization measures instruction records, address-slot frequencies, spatial locality proxies, sampled address-history reuse, and window variability.

It does **not** directly measure cache-hit rates, cache misses, prefetcher accuracy, or speedup.

Four sampled windows from a single trace are descriptive observations, not independent workload repetitions.

## Earlier hardware-prefetcher evaluation

An earlier study evaluated **five workloads only**: FFT, FIR, convolution, matrix multiplication, and IIR.

Those experiments compared NOPREF, IPCP, BERTI, and PYTHIA under the recorded Athena evaluation protocol.

| Workload | NOPREF IPC | IPCP IPC | BERTI IPC | PYTHIA IPC |
|---|---:|---:|---:|---:|
| FFT | 1.58277 | 1.60032 | 1.63517 | 1.63849 |
| FIR | 0.96770 | 0.96817 | 0.98347 | 0.97738 |
| Convolution | 1.57119 | 1.74082 | 1.74230 | 1.66645 |
| Matrix multiplication | 0.67875 | 0.99799 | 0.99688 | 0.68020 |
| IIR | 0.30975 | 0.51909 | 0.51239 | 0.51182 |

These are earlier prefetcher simulation results, separate from the 13-workload characterization study.

**The eight subsequently added workloads are not claimed to have undergone the same four-prefetcher comparison.**

Original evidence is retained in [results/](results/), including [FINAL_RESULTS_TABLE.txt](results/FINAL_RESULTS_TABLE.txt) and [MASTER_RESULTS.csv](results/MASTER_RESULTS.csv).

## Trace format

Traces are distributed as XZ-compressed binary files.

Each decompressed trace contains fixed **64-byte dynamic instruction records** using the project's Athena-compatible format.

Relevant offsets:

| Field | Byte offset |
|---|---:|
| Instruction pointer (uint64) | 0 |
| Branch flag | 8 |
| Branch-taken flag | 9 |
| Destination memory addresses (two uint64 slots) | 16 |
| Source memory addresses (four uint64 slots) | 32 |

Address slots with zero values represent unused entries according to the recorded trace convention.

A trace's raw file size divided by 64 gives its dynamic instruction-record count.

The trace representation does not provide every property of an actual processor memory hierarchy, such as complete access sizes or cache residency.

## Reproducibility and downloads

| Resource | Location |
|---|---|
| All compressed traces | [traces/](traces/) |
| All workload source files | [workloads/](workloads/) |
| Individual characterization reports | [characterization/workloads/](characterization/workloads/) |
| Complete full-trace statistics | [full_trace_summary.csv](characterization/data/full_trace_summary.csv) |
| Four-window temporal results | [temporal_windows.csv](characterization/data/temporal_windows.csv) |
| Hot-line sensitivity data | [hotline_sensitivity_windows.csv](characterization/data/hotline_sensitivity_windows.csv) |
| Summary findings | [workload_scientific_summary.csv](characterization/data/workload_scientific_summary.csv) |
| Research figures | [characterization/figures/](characterization/figures/) |
| Original characterization scripts | [analysis/](analysis/) |
| Intel Pin tracer sources | [tracer/](tracer/) |
| All compressed-trace SHA256 hashes | [ALL13_COMPRESSED_SHA256SUMS.txt](checksums/ALL13_COMPRESSED_SHA256SUMS.txt) |
| Earlier prefetcher results | [results/](results/) |

The seven programs under `analysis/as_run/` are preserved exactly as originally used. Some contain environment-specific absolute paths and require prior outputs or decompressed traces.

**They are historical implementation evidence, not yet a portable one-command reproduction pipeline.** Do not rerun them against frozen research directories.

## Trace-generation and provenance notes

Intel Pin was used for dynamic binary instrumentation. Intel Pin itself is not included.

Athena upstream:
https://github.com/CMU-SAFARI/Athena

Previously recorded frozen Athena commit:
`1cf9be66d1e9449a040ea2dda944af36fdae3f28`

The repository includes a baseline tracer and two separately frozen compatibility variants:

- CP22 Polyphase: IDIV compatibility tracer
- CP23 STFT: REP MOVS compatibility tracer

See [tracer/VARIANTS.md](tracer/VARIANTS.md).

**AEC/NLMS provenance exception:** Its accepted trace originated as a one-repetition sizing pilot, later selected as the canonical trace through separate frozen acceptance evidence. A new independent final trace-generation run is not claimed.

See [CP26 acceptance provenance](docs/CP26_ACCEPTANCE_PROVENANCE.md).

## Research scope and limitations

This is a collection of 13 accepted workload traces, not a representative statistical sample of all embedded applications.

No measured SPEC-versus-DSP comparison, general superiority over standard benchmark suites, or additional prefetcher-performance improvement is established by the new characterization results alone.

The project is an ongoing research effort; the remaining seven workloads in the proposed 20-workload suite are incomplete.

## Licensing

No project-wide license has yet been selected. Access to these artifacts does not by itself grant general redistribution or modification rights. Reuse terms, including those of external dependencies, must be reviewed before a public research release.
