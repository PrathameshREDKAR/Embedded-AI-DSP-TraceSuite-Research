# Intel Pin Tracer Versions

The repository contains the original baseline Intel Pin
tracer and two separately frozen compatibility variants.

| Source | Role |
|---|---|
| athena_tracer.cpp | Existing baseline tracer |
| athena_tracer_cp22_idiv_compat.cpp | CP22 Polyphase IDIV compatibility |
| athena_tracer_cp23_repmovs_compat.cpp | CP23 STFT REP MOVS compatibility |

## Verified variant hashes

CP22:
b1560a1da8dc4e471ee3a1658f1117ce052d2bd08e2711e45af498cf52e46821

CP23:
d31493a50203ca76f9fe936ae1272e38bdb3cd25934bc3391c2110d51a0180ef

The compatibility sources were copied byte-identically
from frozen research evidence.

No new tracer build or trace generation was performed for
this GitHub release.

Intel Pin is not redistributed.

The source files alone do not establish all historical
executed-binary identities; original run-specific records
remain authoritative.
