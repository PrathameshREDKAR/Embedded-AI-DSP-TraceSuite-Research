import hashlib
import json
import lzma
import sys
import time
from pathlib import Path
import numpy as np

ROOT = Path(r"D:\Embedded_AI_DSP_TraceSuite")
INPUT = ROOT / "CP26_AEC_NLMS" / "CP26_AEC_NLMS_ALTERNATIVE_1X.xz"
OUTPUT = ROOT / "ACCML_CHAR_20261109_V1" / "AEC_NLMS_provisional_full_v1.json"

EXPECTED_XZ_SHA256 = "92791ee4c4ffeb357f8302ecd8143cc79e42c73c4bf6d98ff6563d045c5330b7"

DTYPE = np.dtype({
    "names": ["ip", "branch", "taken", "dst", "src"],
    "formats": ["<u8", "u1", "u1", ("<u8", 2), ("<u8", 4)],
    "offsets": [0, 8, 9, 16, 32],
    "itemsize": 64
})

CHUNK_BYTES = 131072 * 64
MAX_RAW_BYTES = 32 * 1024**3
MAX_SECONDS = 1800
WINDOW_RECORDS = 100000

start = time.monotonic()

def check_time():
    if time.monotonic() - start > MAX_SECONDS:
        raise RuntimeError("TIME_BUDGET_EXCEEDED")

def input_chunks():
    with lzma.open(INPUT, "rb") as stream:
        while True:
            check_time()
            block = stream.read(CHUNK_BYTES)
            if not block:
                break
            if len(block) % 64:
                raise RuntimeError("INCOMPLETE_RECORD")
            yield block

def main():
    if not INPUT.is_file() or INPUT.is_symlink():
        raise RuntimeError("INPUT_MISSING_OR_SYMLINK")
    if OUTPUT.exists():
        raise RuntimeError("OUTPUT_ALREADY_EXISTS")

    sha = hashlib.sha256()
    with INPUT.open("rb") as stream:
        while True:
            block = stream.read(1024 * 1024)
            if not block:
                break
            sha.update(block)

    compressed_hash = sha.hexdigest()
    if compressed_hash != EXPECTED_XZ_SHA256:
        raise RuntimeError("COMPRESSED_HASH_MISMATCH")

    print("COMPRESSED_HASH_MATCH=True", flush=True)

    # Pass 1: discover full decompressed length.
    raw_bytes = 0
    for block in input_chunks():
        raw_bytes += len(block)
        if raw_bytes > MAX_RAW_BYTES:
            raise RuntimeError("DECOMPRESSED_SIZE_LIMIT")

    if raw_bytes == 0 or raw_bytes % 64:
        raise RuntimeError("INVALID_DECOMPRESSED_SIZE")

    records = raw_bytes // 64
    print("FULL_RECORDS={}".format(records), flush=True)
    print("FULL_RAW_BYTES={}".format(raw_bytes), flush=True)

    phase_starts = [records * i // 4 for i in range(4)]
    phase_lines = [set() for _ in range(4)]

    result = {
        "workload": "AEC_NLMS",
        "cohort": "PROVISIONAL_CP26_ALTERNATIVE_1X",
        "acceptance_verified": False,
        "record_bytes": 64,
        "cache_line_bytes": 64,
        "records": records,
        "raw_bytes": raw_bytes,
        "compressed_sha256": compressed_hash,
        "load_slots": 0,
        "store_slots": 0,
        "instructions_with_loads": 0,
        "instructions_with_stores": 0,
        "branches": 0,
        "zero_ip": 0,
        "invalid_flags": 0
    }

    metrics = {}
    prev = {"load": None, "store": None}
    for kind in ("load", "store"):
        for category in (
            "zero", "plus8", "minus8", "plus64",
            "minus64", "same_line", "transitions"
        ):
            metrics[kind + "_" + category] = 0

    raw_sha = hashlib.sha256()
    processed = 0
    last_progress = 0

    # Pass 2: full-trace analysis.
    for block in input_chunks():
        raw_sha.update(block)
        rec = np.frombuffer(block, dtype=DTYPE)
        n = len(rec)

        src = rec["src"]
        dst = rec["dst"]

        result["zero_ip"] += int(np.count_nonzero(rec["ip"] == 0))
        result["branches"] += int(np.count_nonzero(rec["branch"] == 1))
        result["invalid_flags"] += int(np.count_nonzero(
            (rec["branch"] > 1) | (rec["taken"] > 1)
        ))

        result["load_slots"] += int(np.count_nonzero(src))
        result["store_slots"] += int(np.count_nonzero(dst))
        result["instructions_with_loads"] += int(
            np.count_nonzero(np.any(src != 0, axis=1))
        )
        result["instructions_with_stores"] += int(
            np.count_nonzero(np.any(dst != 0, axis=1))
        )

        for kind, matrix in (("load", src), ("store", dst)):
            addresses = matrix.reshape(-1)
            addresses = addresses[addresses != 0]

            if not len(addresses):
                continue

            if prev[kind] is None:
                left = addresses[:-1]
                right = addresses[1:]
            else:
                extended = np.empty(
                    len(addresses) + 1, dtype=np.uint64
                )
                extended[0] = prev[kind]
                extended[1:] = addresses
                left = extended[:-1]
                right = extended[1:]

            diff = right - left
            prev[kind] = int(addresses[-1])
            p = kind + "_"

            metrics[p + "transitions"] += len(diff)
            metrics[p + "zero"] += int(np.count_nonzero(diff == 0))
            metrics[p + "plus8"] += int(np.count_nonzero(diff == 8))
            metrics[p + "minus8"] += int(np.count_nonzero(
                diff == np.uint64(2**64 - 8)
            ))
            metrics[p + "plus64"] += int(np.count_nonzero(diff == 64))
            metrics[p + "minus64"] += int(np.count_nonzero(
                diff == np.uint64(2**64 - 64)
            ))
            metrics[p + "same_line"] += int(np.count_nonzero(
                (left >> 6) == (right >> 6)
            ))

        for i, begin in enumerate(phase_starts):
            lo = max(begin, processed)
            hi = min(begin + WINDOW_RECORDS, processed + n)

            if hi <= lo:
                continue

            a = lo - processed
            b = hi - processed

            for matrix in (src[a:b], dst[a:b]):
                values = matrix.reshape(-1)
                values = values[values != 0]
                if len(values):
                    phase_lines[i].update(
                        int(x) for x in np.unique(values >> 6)
                    )

        processed += n

        if processed - last_progress >= 10000000:
            print(
                "PROGRESS_RECORDS={} OF={}".format(
                    processed, records
                ), flush=True
            )
            last_progress = processed

    if processed != records:
        raise RuntimeError("FINAL_RECORD_COUNT_MISMATCH")
    if result["invalid_flags"] or result["zero_ip"]:
        raise RuntimeError("INVALID_TRACE_RECORD_FIELDS")

    result.update(metrics)
    result["raw_sha256"] = raw_sha.hexdigest()
    result["memory_address_slots"] = (
        result["load_slots"] + result["store_slots"]
    )

    if result["memory_address_slots"] == 0:
        raise RuntimeError("NO_MEMORY_ADDRESSES")

    result["load_fraction"] = (
        result["load_slots"] / result["memory_address_slots"]
    )
    result["branch_fraction"] = result["branches"] / records
    result["memory_slots_per_instruction"] = (
        result["memory_address_slots"] / records
    )

    for kind, slots in (
        ("load", result["load_slots"]),
        ("store", result["store_slots"])
    ):
        p = kind + "_"
        count = metrics[p + "transitions"]

        if count != slots - 1 or count <= 0:
            raise RuntimeError("STRIDE_TRANSITION_MISMATCH")

        result[p + "same_line_fraction"] = (
            metrics[p + "same_line"] / count
        )
        result[p + "plus8_fraction"] = (
            metrics[p + "plus8"] / count
        )

    for i in range(4):
        result["window{}_unique_lines".format(i + 1)] = len(
            phase_lines[i]
        )

    result["elapsed_seconds"] = round(
        time.monotonic() - start, 3
    )
    result["status"] = "COMPLETE_PROVISIONAL"

    with OUTPUT.open("x", encoding="utf-8") as f:
        json.dump(result, f, indent=2)
        f.write("\n")

    print("=== CHAR-006 RESULT ===")
    print("WORKLOAD=AEC_NLMS")
    print("RECORDS={}".format(records))
    print("LOAD_PCT={:.3f}".format(100 * result["load_fraction"]))
    print("BRANCH_PCT={:.3f}".format(100 * result["branch_fraction"]))
    print("LOAD_SAME_LINE_PCT={:.3f}".format(
        100 * result["load_same_line_fraction"]
    ))
    print("RAW_SHA256={}".format(result["raw_sha256"]))
    print("OUTPUT_SHA256={}".format(
        hashlib.sha256(OUTPUT.read_bytes()).hexdigest()
    ))
    print("ACCEPTANCE_VERIFIED=NO")
    print("CHAR006_STATUS=PASS")
    return 0

if __name__ == "__main__":
    try:
        sys.exit(main())
    except Exception as exc:
        print("CHAR006_STATUS=FAIL")
        print("ERROR_TYPE={}".format(type(exc).__name__))
        print("ERROR_MESSAGE={}".format(str(exc)))
        print("AUTOMATIC_RETRY=NO")
        sys.exit(2)
