# Embedded AI/DSP Trace Characterization

This section documents **13 accepted instruction traces** containing
**1,274,683,876 dynamic instruction records** in total.
The traces use 64-byte Athena/ChampSim-style instruction records.

## Workload results

The full-trace metrics below are measured on complete traces.
The final column is from four sampled windows per workload.

| Workload | Instructions | Branch fraction | Same-line load fraction | Hot-line exclusion effect |
|---|---:|---:|---:|---:|
| [FFT](workloads/FFT/README.md) | 69,607,705 | 6.04% | 70.72% | -14.14 pp |
| [FIR](workloads/FIR/README.md) | 68,105,642 | 10.16% | 55.75% | -0.52 pp |
| [CONV](workloads/CONV/README.md) | 65,426,935 | 10.06% | 53.23% | -4.90 pp |
| [MATMUL](workloads/MATMUL/README.md) | 101,321,253 | 8.41% | 55.61% | -0.21 pp |
| [IIR](workloads/IIR/README.md) | 62,390,447 | 11.76% | 37.50% | -10.43 pp |
| [DCT](workloads/DCT/README.md) | 63,009,469 | 8.55% | 29.34% | -2.63 pp |
| [DWT](workloads/DWT/README.md) | 64,254,895 | 8.22% | 66.52% | -9.73 pp |
| [LMS](workloads/LMS/README.md) | 64,722,031 | 10.82% | 38.94% | -0.09 pp |
| [POLYPHASE](workloads/POLYPHASE/README.md) | 69,065,955 | 15.53% | 64.58% | -1.01 pp |
| [STFT](workloads/STFT/README.md) | 80,714,506 | 5.75% | 59.43% | -1.47 pp |
| [MFCC](workloads/MFCC/README.md) | 114,614,807 | 3.21% | 55.12% | -4.22 pp |
| [BEAMFORM](workloads/BEAMFORM/README.md) | 346,893,926 | 18.16% | 48.68% | -6.67 pp |
| [AEC_NLMS](workloads/AEC_NLMS/README.md) | 104,556,305 | 8.09% | 52.31% | +1.57 pp |

## Key observed findings

1. The consecutive-load same-64-byte-line fraction ranges from
   approximately 29.34% (DCT) to 70.72% (FFT).
2. MFCC's sampled unique-line counts alternate between 1,257 and 144,
   demonstrating window-to-window variation. This is not yet a
   verified mapping to algorithmic phases.
3. Removing the ten hottest lines changes measured recent reuse
   substantially for some workloads (for example, FFT and IIR).
   Removing them changes the reference stream and does not produce
   a corrected cache-hit rate.

## Data and figures

- [Full-trace summary](data/full_trace_summary.csv)
- [Four-window temporal statistics](data/temporal_windows.csv)
- [Hot-line sensitivity data](data/hotline_sensitivity_windows.csv)
- [Workload-level scientific summary](data/workload_scientific_summary.csv)
- [Trace catalog and checksums](TRACE_CATALOG.csv)
- [Figures](figures/)
- [Original audit records](evidence/)
- [Scientific findings text](evidence/CHAR012_SCIENTIFIC_FINDINGS.txt)

## Measurement definitions and limitations

- Load fraction: nonzero load-address slots divided by all nonzero
  memory-address slots; **not** percentage of instructions with loads.
- Branch fraction: flagged branch instruction records divided by
  the total instruction record count.
- Same-line locality: consecutive recorded load addresses mapping
  to the same assumed 64-byte line.
- Recent reuse: an address line appearing in the prior 1,024
  recorded memory-address slots within a sampled window.
- Hot-line sensitivity: remove each window's ten most frequently
  referenced lines and recompute recent reuse on the remaining stream.
- Four samples per workload, each spanning 100,000 instructions,
  were collected near four locations in its original trace.
- Memory access sizes and physical-address cache residency are
  unavailable from this trace ABI.
- No performance improvement, general-population statistical result,
  or application-stage boundary is established by this analysis.

## Provenance

- Source evidence archive: `CHAR013_ACCML_HANDOFF.zip`
- Source archive SHA256:
  `9a6f2c0beed197c4b7c0c5fd42962f510965365ea329d3d8f7d05ea0c8b80007`
- Original full-scan JSON results are preserved under each
  workload directory without altering their recorded status.
- AEC/NLMS was retrospectively accepted as the canonical trace
  after a 1x sizing pilot. Its earlier JSON still records a
  provisional status and must be interpreted alongside later
  frozen acceptance documentation.
- Portable analyzer instructions and workload source provenance
  will be added and audited before public publication.

**Research status:** Characterization complete for these 13 traces.
The broader 20-workload generation suite is still incomplete.
