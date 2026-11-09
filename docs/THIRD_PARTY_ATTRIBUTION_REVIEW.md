# Third-Party Attribution and Licensing Review

Status: PRE-PUBLICATION REVIEW DOCUMENT
Project-wide license: NOT YET SELECTED
Public release authorization: PENDING

This document distinguishes original research artifacts
from software components associated with external projects.

It does not itself grant permission to copy, modify,
redistribute, or sublicense any component.

## 1. Athena simulator

Upstream project:
https://github.com/CMU-SAFARI/Athena

Associated publication:

Rahul Bera et al.,
"Athena: Synergizing Data Prefetching and Off-Chip
Prediction via Online Reinforcement Learning",
HPCA 2026.

Recorded upstream commit:

1cf9be66d1e9449a040ea2dda944af36fdae3f28

The upstream Athena README at that commit states that
the project is distributed under the MIT License.

The referenced upstream LICENSE file was not retrieved
during this review. The applicable license text and
copyright notices must be verified before public release.

Included Athena-related derivative:

athena/compatibility_patch/ooo_cpu.cc

SHA256:
3c3465f1efded7d602489ab4d42cbef644dd4ea528d390874c5fa114052d01cd

The compatibility source is provided to document the
earlier five-workload simulator environment. It must
retain appropriate Athena attribution and applicable
third-party license notices.

The upstream Athena source tree is not bundled here.

## 2. Intel Pin

Official project information:
https://www.intel.com/content/www/us/en/developer/articles/tool/pin-a-dynamic-binary-instrumentation-tool.html

The included tracer programs depend on Intel Pin.

Intel describes the Pin instrumentation engine as
governed by the Intel Simplified Software License.

Intel describes source code in the Pin kit, including
sample code and headers, as governed by the MIT License.

The Pin engine and Pin kit are not redistributed in
this research release candidate.

Included tracer programs:

- tracer/athena_tracer.cpp
- tracer/athena_tracer_cp22_idiv_compat.cpp
- tracer/athena_tracer_cp23_repmovs_compat.cpp

The authorship and any derivation from Intel Pin sample
code or other tracer implementations require confirmation.

A dependency on Intel Pin is not itself proof that
the tracer implementation is original or third-party.

## 3. Workload programs

The release contains 13 C++ workload implementations.

The source audit identified standard C/C++ includes
and no explicit license notices in these files.

Original authorship, any adapted algorithms or source,
and authority to grant redistribution rights must
be confirmed before choosing a workload-code license.

Standard mathematical algorithms are not by themselves
evidence that a particular implementation is copied.

## 4. Analysis programs

Seven historical Python characterization scripts are
preserved byte-for-byte with their SHA256 hashes.

Their originality, any incorporated third-party code,
and licensing authority require confirmation.

These scripts are not yet a portable one-command
reproduction pipeline.

## 5. Compressed traces and scientific results

The release candidate contains 13 accepted traces,
characterization CSV and JSON data, figures, and audits.

These datasets do not automatically inherit the license
of Athena or Intel Pin merely because the tools were
used during generation.

The project team must decide what reuse and attribution
terms to grant for original research data, subject to
any applicable third-party rights.

## 6. Required decisions before public release

- Confirm authorship and permission for workload sources
- Confirm origin and modification rights for tracer sources
- Preserve Athena and other applicable upstream notices
- Choose licensing terms for original code
- Choose reuse terms for original trace data and figures
- Complete a final clean-release content audit
- Obtain explicit authorization before creating,
  uploading, or making a GitHub repository public

No license selection or public-release authorization
is implied by this document.
