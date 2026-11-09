# Athena Compatibility Patch

This directory documents the Athena compatibility source
used in the earlier five-workload DSP prefetcher evaluation.

## Original Athena baseline

Upstream repository:
https://github.com/CMU-SAFARI/Athena

Recorded frozen upstream commit:

`1cf9be66d1e9449a040ea2dda944af36fdae3f28`

The original Athena source tree is not redistributed
as part of this release candidate.

The source checksum manifest is retained at:

`source_identity/ATHENA_SOURCE_MANIFEST_SHA256.txt`

## Purpose of the compatibility patch

During FIR trace replay, pseudo or special registers in
generated instruction traces could participate in register
dependency tracking.

The compatibility source modifies the relevant register
dependency handling to apply the intended filtering.

No workload trace was regenerated as part of this patch.

## Compatibility file

`compatibility_patch/ooo_cpu.cc`

SHA256:

`3c3465f1efded7d602489ab4d42cbef644dd4ea528d390874c5fa114052d01cd`

## Reproduction outline

1. Obtain the upstream Athena source separately.
2. Check out the recorded frozen commit.
3. Replace `src/ooo_cpu.cc` with the compatibility
   source provided here.
4. Build using the applicable upstream build procedure.
5. Verify the patched source against the SHA256 above.

These are reproduction instructions, not a claim that
the full historical experiment has been rerun for this release.

## Historical evidence and disclosure scope

The original frozen source-identity attestation and the
CP13 compatibility smoke-test transcript are retained
in the private research evidence, not in this clean
release candidate.

Both were deliberately omitted because they contain
machine-specific user-profile paths.

The preserved checksum manifest and compatibility
source document the recorded source baseline and patch.
They do not replace the complete private execution logs.

## Licensing

Upstream Athena licensing and any external dependency
notices must be respected. The project-wide public
release license has not yet been selected.
