import csv
import hashlib
import json
import os
import sys
import time
from pathlib import Path
import numpy as np

BASE = Path(r"C:\DSPTraceData\Embedded_AI_DSP_TraceSuite\final")
OUT = Path(r"D:\Embedded_AI_DSP_TraceSuite\ACCML_CHAR_20261109_V1")

TRACES = [
    ("FFT", "CP12_FFT_100x", 4454893120),
    ("FIR", "CP13_FIR_13x", 4358761088),
    ("CONV", "CP14_CONV_3x", 4187323840),
    ("MATMUL", "CP15_MATMUL_2x", 6484560192),
    ("IIR", "CP16_IIR_14x", 3992988608),
    ("DCT", "CP19_DCT_2x", 4032606016),
    ("DWT", "CP20_DWT_8x", 4112313280),
    ("LMS", "CP21_LMS_3X", 4142209984),
    ("POLYPHASE", "CP22_POLYPHASE_4X", 4420221120),
    ("STFT", "CP23_STFT_3X", 5165728384),
    ("MFCC", "CP24_MFCC_2X", 7335347648),
    ("BEAMFORM", "CP25_DELAY_SUM_BEAMFORMING_1X", 22201211264),
]

DTYPE = np.dtype({
    "names": ["ip", "branch", "taken", "dst", "src"],
    "formats": ["<u8", "u1", "u1", ("<u8", 2), ("<u8", 4)],
    "offsets": [0, 8, 9, 16, 32],
    "itemsize": 64
})

CHUNK_RECORDS = 131072
WINDOW_RECORDS = 100000
MAX_CAMPAIGN_SECONDS = 4 * 3600
start_time = time.monotonic()

def new_json(path, obj):
    with open(path, "x", encoding="utf-8") as f:
        json.dump(obj, f, indent=2)
        f.write("\n")

def analyze(name, folder, expected_bytes):
    path = BASE / folder / "athena_trace.bin"
    if not path.is_file() or path.is_symlink():
        raise RuntimeError("INVALID_INPUT_PATH:" + str(path))
    if path.stat().st_size != expected_bytes:
        raise RuntimeError("RAW_BYTE_LENGTH_MISMATCH:" + name)

    expected_records = expected_bytes // 64
    phase_starts = [expected_records * p // 4 for p in range(4)]
    phase_lines = [set() for _ in range(4)]

    result = {
        "workload": name,
        "cohort": "FINAL_TRACE_CANDIDATE",
        "raw_path": str(path),
        "expected_records": expected_records,
        "record_bytes": 64,
        "cache_line_bytes": 64,
        "load_slots": 0,
        "store_slots": 0,
        "instructions_with_loads": 0,
        "instructions_with_stores": 0,
        "branches": 0,
        "zero_ip": 0,
        "invalid_flags": 0
    }

    deltas = {}
    previous = {"load": None, "store": None}
    for kind in ("load", "store"):
        for key in ("zero", "plus8", "minus8", "plus64",
                    "minus64", "same_line", "transitions"):
            deltas[kind + "_" + key] = 0

    sha = hashlib.sha256()
    processed = 0
    started = time.monotonic()
    next_progress = 10000000

    with open(path, "rb", buffering=1024 * 1024) as f:
        while True:
            if time.monotonic() - start_time > MAX_CAMPAIGN_SECONDS:
                raise RuntimeError("CAMPAIGN_TIME_BUDGET_REACHED")

            data = f.read(CHUNK_RECORDS * 64)
            if not data:
                break
            if len(data) % 64:
                raise RuntimeError("PARTIAL_RECORD:" + name)

            sha.update(data)
            rec = np.frombuffer(data, dtype=DTYPE)
            n = len(rec)

            result["zero_ip"] += int(np.count_nonzero(rec["ip"] == 0))
            result["branches"] += int(np.count_nonzero(rec["branch"] == 1))
            result["invalid_flags"] += int(np.count_nonzero(
                (rec["branch"] > 1) | (rec["taken"] > 1)
            ))

            src = rec["src"]
            dst = rec["dst"]

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

                if len(addresses) == 0:
                    continue

                if previous[kind] is None:
                    differences = np.diff(addresses)
                    left = addresses[:-1]
                    right = addresses[1:]
                else:
                    extended = np.empty(len(addresses) + 1, dtype=np.uint64)
                    extended[0] = previous[kind]
                    extended[1:] = addresses
                    differences = np.diff(extended)
                    left = extended[:-1]
                    right = extended[1:]

                previous[kind] = int(addresses[-1])

                prefix = kind + "_"
                deltas[prefix + "transitions"] += len(differences)
                deltas[prefix + "zero"] += int(np.count_nonzero(
                    differences == 0
                ))
                deltas[prefix + "plus8"] += int(np.count_nonzero(
                    differences == 8
                ))
                deltas[prefix + "minus8"] += int(np.count_nonzero(
                    differences == np.uint64(2**64 - 8)
                ))
                deltas[prefix + "plus64"] += int(np.count_nonzero(
                    differences == 64
                ))
                deltas[prefix + "minus64"] += int(np.count_nonzero(
                    differences == np.uint64(2**64 - 64)
                ))
                deltas[prefix + "same_line"] += int(np.count_nonzero(
                    (left >> 6) == (right >> 6)
                ))

            # Four position-based windows; bounded sampling only.
            for p, begin in enumerate(phase_starts):
                end = begin + WINDOW_RECORDS
                lo = max(begin, processed)
                hi = min(end, processed + n)
                if hi <= lo:
                    continue

                local_lo = lo - processed
                local_hi = hi - processed
                window_src = src[local_lo:local_hi].reshape(-1)
                window_dst = dst[local_lo:local_hi].reshape(-1)

                for addresses in (window_src, window_dst):
                    valid = addresses[addresses != 0]
                    if len(valid):
                        phase_lines[p].update(
                            int(x) for x in np.unique(valid >> 6)
                        )

            processed += n

            if processed >= next_progress:
                print(
                    "PROGRESS={} RECORDS={} OF={} ELAPSED_S={:.1f}".format(
                        name, processed, expected_records,
                        time.monotonic() - started
                    ), flush=True
                )
                next_progress += 10000000

    if processed != expected_records:
        raise RuntimeError("RECORD_COUNT_MISMATCH:" + name)
    if result["invalid_flags"]:
        raise RuntimeError("INVALID_BRANCH_FLAGS:" + name)

    result["records"] = processed
    result["raw_sha256"] = sha.hexdigest()
    result["memory_address_slots"] = (
        result["load_slots"] + result["store_slots"]
    )
    result["load_fraction"] = (
        result["load_slots"] / result["memory_address_slots"]
        if result["memory_address_slots"] else None
    )
    result["memory_slots_per_instruction"] = (
        result["memory_address_slots"] / processed
    )
    result["branch_fraction"] = result["branches"] / processed
    result.update(deltas)

    for kind in ("load", "store"):
        count = deltas[kind + "_transitions"]
        result[kind + "_same_line_fraction"] = (
            deltas[kind + "_same_line"] / count if count else None
        )
        result[kind + "_plus8_fraction"] = (
            deltas[kind + "_plus8"] / count if count else None
        )

    for p in range(4):
        result["window{}_unique_lines".format(p + 1)] = len(
            phase_lines[p]
        )

    result["elapsed_seconds"] = round(
        time.monotonic() - started, 3
    )
    result["status"] = "COMPLETE"
    return result

def main():
    if not OUT.is_dir():
        raise RuntimeError("OUTPUT_DIRECTORY_MISSING")

    print("NUMPY_VERSION={}".format(np.__version__), flush=True)
    print("CHUNK_RECORDS={}".format(CHUNK_RECORDS), flush=True)
    print("TOTAL_WORKLOADS={}".format(len(TRACES)), flush=True)
    print("WINDOWS_ARE_FULL_TRACE_UNIQUE_FOOTPRINT=NO", flush=True)

    # Check all inputs before starting measurements.
    for name, folder, expected in TRACES:
        p = BASE / folder / "athena_trace.bin"
        if not p.is_file() or p.is_symlink():
            raise RuntimeError("PREFLIGHT_MISSING:" + name)
        if p.stat().st_size != expected or expected % 64:
            raise RuntimeError("PREFLIGHT_SIZE_MISMATCH:" + name)

    print("ALL_INPUT_PREFLIGHTS=PASS", flush=True)
    completed = 0

    for name, folder, size in TRACES:
        print("START_WORKLOAD={}".format(name), flush=True)

        try:
            result = analyze(name, folder, size)
            new_json(OUT / (name + "_full_v1.json"), result)
            completed += 1
            print(
                "COMPLETE_WORKLOAD={} RECORDS={} LOAD_FRAC={:.6f} "
                "BRANCH_FRAC={:.6f} RAW_SHA256={} ELAPSED_S={}".format(
                    name, result["records"],
                    result["load_fraction"], result["branch_fraction"],
                    result["raw_sha256"], result["elapsed_seconds"]
                ), flush=True
            )
        except Exception as exc:
            failure = {
                "workload": name,
                "error_type": type(exc).__name__,
                "error": str(exc),
                "completed_before_failure": completed
            }
            new_json(OUT / (name + "_failure_v1.json"), failure)
            print("WORKLOAD_FAILED={} ERROR={}".format(
                name, str(exc)
            ), flush=True)
            print("AUTOMATIC_RETRY=NO", flush=True)
            return 2

    print("=== CHAR-004 FINAL VERDICT ===", flush=True)
    print("FULL_WORKLOADS_COMPLETED={}".format(completed), flush=True)
    print("FULL_WORKLOADS_EXPECTED=12", flush=True)
    print("CP26_AEC_NLMS=PROVISIONAL_EXCLUDED", flush=True)
    print("ORIGINAL_TRACES_MODIFIED=NO", flush=True)
    print("CHAR004_STATUS=PASS", flush=True)
    return 0

if __name__ == "__main__":
    sys.exit(main())
