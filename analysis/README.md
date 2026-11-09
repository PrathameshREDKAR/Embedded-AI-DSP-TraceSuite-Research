# Original Trace Characterization Scripts

These seven Python files are the original analysis programs
used for the November 2026 Embedded AI/DSP trace study.

The `as_run/` files are preserved byte-for-byte from the
verified CHAR013 research handoff.

## Analysis coverage

- Full-trace characterization
- Arithmetic auditing and figures
- CP26 provisional full-trace measurement
- Thirteen-workload consolidation
- Four-window temporal locality analysis
- Ten-hot-line exclusion sensitivity
- Scientific findings consolidation

## Reproducibility limitations

These are historical as-run scripts, not a portable automated
pipeline.

Some scripts include absolute C: or D: filesystem paths,
expect decompressed trace files, and depend on earlier
analysis outputs.

Do not run these scripts against original frozen research
directories. They are published to document the method.

The measured CSV files, original JSON measurements, audits,
and figures are under `../characterization/`.

See `AS_RUN_SHA256SUMS.txt` for script integrity hashes.

## Scientific interpretation

Recent address reuse is not a cache-hit rate.

Filtering the ten hottest cache lines alters the address
stream and does not represent actual cache eviction.

Four sampled windows are descriptive observations from each
trace, not independent experimental replicates.
