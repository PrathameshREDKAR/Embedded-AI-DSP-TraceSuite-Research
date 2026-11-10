# Prospective DSP Prefetching Pilot (2026)

This directory is reserved for the **separate, prospective, single-core 16-arm prefetching pilot** described in the GigaFetcher / Edge-AI DSP-kernel study. It must not be conflated with the repository's earlier five-workload prefetcher evaluation under `results/`.

## Release status

**Documentation established; original experiment files pending transfer and independent verification.**

The status below is reconstructed from the researchers' preserved experiment-chat transcripts. It is **not** a substitute for the original machine-generated logs, run markers, or frozen manifest. Do not claim that the raw prospective pilot is publicly reproduced or archived until those files have actually been added and audited.

| Workload | NOPREF | IPCP (L1D) | Berti (L1D) | Scooby/L2C |
|---|---|---|---|---|
| FFT | Completed | Completed | Completed | Completed |
| Convolution | Completed | Completed | Completed | Completed |
| Matrix multiplication | Completed | Completed | Completed | Completed |
| MFCC | Failed before warm-up completed | Not attempted | Not attempted | Not attempted |

- **16 arms prospectively specified**; **12 successful simulation results** across three kernels.
- The **MFCC NOPREF attempt failed**; the remaining three MFCC arms were not launched under the frozen stop-on-first-failure policy.
- **Single attempt per arm**; no independent replications.
- Simulation: a frozen, isolated **Athena-derived ChampSim** configuration, **one core**, **10 million warm-up instructions** and **50 million ROI instructions**; fixed input trace per workload.
- Treatment labels are NOPREF; IPCP at L1D; Berti at L1D; and **Scooby at L2C using a Pythia-named configuration**. The latter must **not** be described as a reproduction of the original Pythia reinforcement-learning agent.

## Transcript-reported descriptive outcomes

Values are rounded from the preserved research-chat summary and are **not recomputed here from original logs**.

| Workload | NOPREF IPC | IPCP gain | Berti gain | Scooby/L2C gain |
|---|---:|---:|---:|---:|
| FFT | 1.58631 | +1.11% | +3.28% | +3.88% |
| Convolution | 1.57119 | +10.80% | +10.89% | +6.06% |
| Matrix multiplication | 0.67875 | +47.03% | +46.86% | +0.21% |

Descriptive geometric-mean IPC gains over **these three kernels only**: IPCP +18.10%, Berti +18.92%, Scooby/L2C +3.36%.

For matrix multiplication, the transcripts report 4,425,774 L1D prefetch requests for IPCP versus 1,765,617 for Berti (**60.11% fewer**). These are **prefetch-request counters**, not DRAM-bandwidth or energy measurements.

## Protocol provenance reported in prior records

- Frozen protocol ID: `GIGA_P2_PILOT16_V1`
- Frozen protocol filename: `GIGA_P2_PILOT16_PROTOCOL_V1_FROZEN.json`
- Reported protocol SHA-256: `8e9cecf5995b4f9c089bd76bdef6496291b143f6444925e1e5ddcccb79ef3179`
- Frozen Athena source baseline commit: `1cf9be66d1e9449a040ea2dda944af36fdae3f28`
- Reported simulator executable SHA-256: `02b9e0349f23309cfb9907049f62482e94b006f5704577ebac7609452f48845a`
- Original transcript milestones: 12/12 successful logs checked (GIGA-P2-005); counter-source audit completed (GIGA-P2-007).

**These identifiers are cited here as reported in the chat record, not as hashes already independently checked against files committed in this directory.**

## Remaining artifact upload checklist

The release should add **only verified, publication-appropriate copies** of existing artifacts:

1. Frozen prospective protocol manifest, with its original SHA-256 verified.
2. Twelve successful raw simulation logs (and their original byte-level SHA-256 manifest).
3. Preserved MFCC failure log and its original attempt marker; ensure it is clearly marked **failed**, never successful.
4. Existing one-shot attempt markers and configuration/provenance records needed to identify actual experiments, subject to checking for private paths or personal data.
5. Parsed results table and parsing/audit scripts, if found in the original experiment evidence, preserving provenance and distinguishing newly generated documentation from as-run source.
6. A machine-readable inventory of included files and SHA-256 checksums.

Before publication, inspect logs and manifests for machine-local paths, usernames, access tokens, or unrelated confidential information. If a public copy is sanitized, label it and record its **new** SHA-256 separately; do not present a modified copy as byte-identical to the original.

No simulations should be rerun, no original evidence should be overwritten, and existing repository content should remain unchanged.

## Scope

This exploratory pilot does not establish end-to-end ML inference speedup, energy savings, representative performance across DSP or SPEC workloads, or statistically significant performance improvements.
