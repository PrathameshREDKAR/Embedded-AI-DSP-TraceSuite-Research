import os
import csv
import json
import math
import hashlib
import struct
import sys
from pathlib import Path
from collections import Counter, deque

ROOT = Path(r"D:\Embedded_AI_DSP_TraceSuite\ACCML_CHAR_20261109_V1")
BASE = Path(r"C:\DSPTraceData\Embedded_AI_DSP_TraceSuite\final")
SSD = Path(r"D:\Embedded_AI_DSP_TraceSuite")

TRACES = [
    ("FFT", "CP12_FFT_100x"),
    ("FIR", "CP13_FIR_13x"),
    ("CONV", "CP14_CONV_3x"),
    ("MATMUL", "CP15_MATMUL_2x"),
    ("IIR", "CP16_IIR_14x"),
    ("DCT", "CP19_DCT_2x"),
    ("DWT", "CP20_DWT_8x"),
    ("LMS", "CP21_LMS_3X"),
    ("POLYPHASE", "CP22_POLYPHASE_4X"),
    ("STFT", "CP23_STFT_3X"),
    ("MFCC", "CP24_MFCC_2X"),
    ("BEAMFORM", "CP25_DELAY_SUM_BEAMFORMING_1X"),
    ("AEC_NLMS", None)
]

WINDOW = 100000
HISTORY = 1024
RECORD_BYTES = 64
ST = struct.Struct("<6Q")

def fail(message):
    raise RuntimeError(message)

def analyze_window(path, start_record, expected_lines):
    counts = Counter()
    recent = deque()
    recent_counts = Counter()

    accesses = 0
    reuse_hits = 0
    loads = 0
    stores = 0
    zero_ip = 0
    invalid_flags = 0

    with path.open("rb", buffering=1024*1024) as f:
        f.seek(start_record * RECORD_BYTES)
        data = f.read(WINDOW * RECORD_BYTES)

    if len(data) != WINDOW * RECORD_BYTES:
        fail("SHORT_WINDOW_READ")

    view = memoryview(data)

    # Independent scalar parser, separate from NumPy full-trace analyzer.
    for i in range(WINDOW):
        offset = i * RECORD_BYTES

        if struct.unpack_from("<Q", view, offset)[0] == 0:
            zero_ip += 1

        if view[offset+8] > 1 or view[offset+9] > 1:
            invalid_flags += 1

        a = ST.unpack_from(view, offset + 16)

        # Trace execution order within an instruction is not explicitly
        # available. This is a deterministic destination-then-source
        # ordering of recorded memory-address slots.
        for j, address in enumerate(a):
            if address == 0:
                continue

            if j < 2:
                stores += 1
            else:
                loads += 1

            line = address >> 6
            accesses += 1
            counts[line] += 1

            if recent_counts[line] > 0:
                reuse_hits += 1

            recent.append(line)
            recent_counts[line] += 1

            if len(recent) > HISTORY:
                old = recent.popleft()
                recent_counts[old] -= 1
                if recent_counts[old] == 0:
                    del recent_counts[old]

    if invalid_flags or zero_ip:
        fail("INVALID_RECORD_FLAGS_OR_IP")

    unique_lines = len(counts)
    if unique_lines != expected_lines:
        fail(
            "UNIQUE_LINE_MISMATCH expected={} observed={}".format(
                expected_lines, unique_lines
            )
        )

    if accesses == 0:
        fail("NO_MEMORY_ADDRESSES")

    probabilities = [n / accesses for n in counts.values()]
    entropy = -sum(p * math.log2(p) for p in probabilities)
    normalized_entropy = (
        entropy / math.log2(unique_lines)
        if unique_lines > 1 else 0.0
    )

    top10 = sum(n for _, n in counts.most_common(10))

    return {
        "records": WINDOW,
        "memory_address_slots": accesses,
        "load_slots": loads,
        "store_slots": stores,
        "unique_64b_lines": unique_lines,
        "footprint_64b_bytes": unique_lines * 64,
        "recent_1024_line_reuse_fraction": reuse_hits / accesses,
        "top10_line_access_fraction": top10 / accesses,
        "line_entropy_bits": entropy,
        "normalized_line_entropy": normalized_entropy,
        "source_destination_order_note":
            "destination slots then source slots per instruction"
    }

def main():
    rows = []
    checked = 0

    previous = json.loads(
        (ROOT / "CHAR009_AUDIT.json").read_text(encoding="utf-8")
    )
    if previous.get("status") != "PASS" or \
       previous.get("accepted_workloads") != 13:
        fail("CHAR009_AUDIT_GATE")

    for name, folder in TRACES:
        if folder is None:
            path = SSD / "CP26_AEC_NLMS" / "PILOT_1X" / "athena_trace.bin"
            result_file = ROOT / "AEC_NLMS_provisional_full_v1.json"
        else:
            path = BASE / folder / "athena_trace.bin"
            result_file = ROOT / (name + "_full_v1.json")

        old = json.loads(result_file.read_text(encoding="utf-8"))
        records = int(old["records"])
        expected_bytes = records * RECORD_BYTES

        if not path.is_file() or path.is_symlink():
            fail("INPUT_MISSING_OR_SYMLINK:" + name)

        if path.stat().st_size != expected_bytes:
            fail("TRACE_SIZE_MISMATCH:" + name)

        print("START_WORKLOAD={}".format(name), flush=True)

        for phase in range(4):
            start_record = records * phase // 4
            expected_lines = int(
                old["window{}_unique_lines".format(phase+1)]
            )

            measured = analyze_window(
                path, start_record, expected_lines
            )

            row = {
                "workload": name,
                "phase": phase+1,
                "start_record": start_record,
            }
            row.update(measured)
            rows.append(row)
            checked += 1

            print(
                "WINDOW_PASS={} PHASE={} UNIQUE_LINES={} "
                "RECENT_REUSE_PCT={:.3f} TOP10_PCT={:.3f} "
                "NORMALIZED_ENTROPY={:.5f}".format(
                    name, phase+1,
                    measured["unique_64b_lines"],
                    100*measured["recent_1024_line_reuse_fraction"],
                    100*measured["top10_line_access_fraction"],
                    measured["normalized_line_entropy"]
                ),
                flush=True
            )

    if checked != 52:
        fail("EXPECTED_52_WINDOWS")

    csv_path = ROOT / "CHAR010_WINDOWS.csv"
    with csv_path.open("x", newline="", encoding="utf-8") as f:
        writer = csv.DictWriter(f, fieldnames=list(rows[0].keys()))
        writer.writeheader()
        writer.writerows(rows)

    audit = {
        "status": "PASS",
        "workloads": 13,
        "windows_verified": 52,
        "records_per_window": WINDOW,
        "memory_history_accesses": HISTORY,
        "independent_scalar_decoder": True,
        "full_trace_temporal_reuse_claim": False,
        "source_traces_modified": False,
        "limitations": [
            "Only four nonoverlapping sampled windows per trace",
            "Recent reuse is access-history-based, not exact stack distance",
            "Per-instruction source/destination temporal order is unavailable",
            "Unique-line footprint is a window footprint, not a full-trace footprint",
            "Virtual-address trace lines do not imply physical cache residency"
        ]
    }

    audit_path = ROOT / "CHAR010_AUDIT.json"
    with audit_path.open("x", encoding="utf-8") as f:
        json.dump(audit, f, indent=2)
        f.write("\n")

    print("=== CHAR-010 FINAL VERDICT ===")
    print("WINDOWS_VERIFIED={}".format(checked))
    print("WORKLOADS=13")
    print("INDEPENDENT_UNIQUE_LINE_COMPARISONS=52")
    print("CHAR010_STATUS=PASS")

    for p in (csv_path, audit_path):
        print(
            "OUTPUT={} SHA256={}".format(
                p.name,
                hashlib.sha256(p.read_bytes()).hexdigest()
            )
        )

    print("CP27=ON_HOLD")
    return 0

if __name__ == "__main__":
    try:
        sys.exit(main())
    except Exception as e:
        print("CHAR010_STATUS=FAIL")
        print("ERROR_TYPE={}".format(type(e).__name__))
        print("ERROR_MESSAGE={}".format(e))
        print("AUTOMATIC_RETRY=NO")
        sys.exit(2)
