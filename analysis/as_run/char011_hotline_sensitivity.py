import csv
import json
import math
import hashlib
import struct
import sys
from collections import Counter, deque
from pathlib import Path
from xml.sax.saxutils import escape

ROOT = Path(r"D:\Embedded_AI_DSP_TraceSuite\ACCML_CHAR_20261109_V1")
BASE = Path(r"C:\DSPTraceData\Embedded_AI_DSP_TraceSuite\final")
SSD = Path(r"D:\Embedded_AI_DSP_TraceSuite")

FOLDERS = {
    "FFT": "CP12_FFT_100x",
    "FIR": "CP13_FIR_13x",
    "CONV": "CP14_CONV_3x",
    "MATMUL": "CP15_MATMUL_2x",
    "IIR": "CP16_IIR_14x",
    "DCT": "CP19_DCT_2x",
    "DWT": "CP20_DWT_8x",
    "LMS": "CP21_LMS_3X",
    "POLYPHASE": "CP22_POLYPHASE_4X",
    "STFT": "CP23_STFT_3X",
    "MFCC": "CP24_MFCC_2X",
    "BEAMFORM": "CP25_DELAY_SUM_BEAMFORMING_1X",
    "AEC_NLMS": None
}

WINDOW = 100000
HISTORY = 1024
MEMORY = struct.Struct("<6Q")

def check(ok, message):
    if not ok:
        raise RuntimeError(message)

def recent_reuse(lines):
    queue = deque()
    present = Counter()
    hits = 0

    for line in lines:
        if present[line] > 0:
            hits += 1

        queue.append(line)
        present[line] += 1

        if len(queue) > HISTORY:
            old = queue.popleft()
            present[old] -= 1
            if present[old] == 0:
                del present[old]

    return hits / len(lines) if lines else None

def inspect_window(path, start):
    with path.open("rb") as f:
        f.seek(start * 64)
        data = f.read(WINDOW * 64)

    check(len(data) == WINDOW * 64, "SHORT_WINDOW_READ")

    view = memoryview(data)
    lines = []

    for i in range(WINDOW):
        offset = i * 64
        addresses = MEMORY.unpack_from(view, offset + 16)

        for address in addresses:
            if address:
                lines.append(address >> 6)

    check(bool(lines), "EMPTY_ADDRESS_STREAM")

    counts = Counter(lines)
    hottest = {line for line, _ in counts.most_common(10)}
    retained = [line for line in lines if line not in hottest]

    return {
        "address_slots": len(lines),
        "unique_lines": len(counts),
        "hot10_accesses": len(lines) - len(retained),
        "retained_accesses": len(retained),
        "raw_recent_reuse": recent_reuse(lines),
        "hot10_removed_recent_reuse": recent_reuse(retained),
        "hot10_fraction": 1 - len(retained) / len(lines)
    }

def write_svg(rows):
    names = list(FOLDERS)
    grouped = {}

    for name in names:
        entries = [r for r in rows if r["workload"] == name]
        check(len(entries) == 4, name + "_PHASE_COUNT")

        grouped[name] = (
            sum(r["raw_recent_reuse"] for r in entries) / 4,
            sum(r["hot10_removed_recent_reuse"]
                for r in entries) / 4
        )

    lines = [
        '<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 1000 710">',
        '<rect width="1000" height="710" fill="white"/>',
        '<text x="155" y="35" font-family="Arial" font-size="22" font-weight="bold">Recent reuse: all lines versus excluding top ten</text>',
        '<text x="155" y="58" font-family="Arial" font-size="13">Four sampled windows per workload; 1,024-address history</text>',
        '<rect x="155" y="665" width="15" height="12" fill="#444"/>',
        '<text x="178" y="676" font-family="Arial" font-size="13">Original</text>',
        '<rect x="290" y="665" width="15" height="12" fill="#aaa"/>',
        '<text x="313" y="676" font-family="Arial" font-size="13">After excluding top ten lines</text>'
    ]

    x0 = 155
    bar_width = 670

    for tick in range(0, 101, 20):
        x = x0 + bar_width * tick / 100
        lines.append(
            '<line x1="{:.1f}" y1="83" x2="{:.1f}" y2="625" stroke="#ddd"/>'.format(x, x)
        )
        lines.append(
            '<text x="{:.1f}" y="646" text-anchor="middle" font-family="Arial" font-size="12">{}%</text>'.format(x, tick)
        )

    for i, name in enumerate(names):
        y = 90 + i * 40
        a, b = grouped[name]

        lines.append(
            '<text x="143" y="{}" text-anchor="end" font-family="Arial" font-size="12">{}</text>'.format(
                y + 17, escape(name)
            )
        )

        for fraction, offset, color in (
            (a, 0, "#444"), (b, 13, "#aaa")
        ):
            lines.append(
                '<rect x="155" y="{}" width="{:.2f}" height="11" fill="{}"/>'.format(
                    y + offset, bar_width * fraction, color
                )
            )

        lines.append(
            '<text x="845" y="{}" font-family="Arial" font-size="12">{:.1f}% / {:.1f}%</text>'.format(
                y + 17, a * 100, b * 100
            )
        )

    lines.append('</svg>')

    with (ROOT / "CHAR011_REUSE_SENSITIVITY.svg").open(
        "x", encoding="utf-8"
    ) as f:
        f.write("\n".join(lines) + "\n")

def main():
    previous = json.loads(
        (ROOT / "CHAR010_AUDIT.json").read_text(encoding="utf-8")
    )

    check(
        previous.get("status") == "PASS"
        and previous.get("windows_verified") == 52,
        "CHAR010_GATE"
    )

    reference = {}

    with (ROOT / "CHAR010_WINDOWS.csv").open(
        newline="", encoding="utf-8"
    ) as f:
        for r in csv.DictReader(f):
            reference[(r["workload"], int(r["phase"]))] = r

    check(len(reference) == 52, "REFERENCE_WINDOW_COUNT")

    rows = []

    for name, folder in FOLDERS.items():
        if folder is None:
            path = SSD / "CP26_AEC_NLMS" / "PILOT_1X" / "athena_trace.bin"
            saved = ROOT / "AEC_NLMS_provisional_full_v1.json"
        else:
            path = BASE / folder / "athena_trace.bin"
            saved = ROOT / (name + "_full_v1.json")

        check(path.is_file() and not path.is_symlink(),
              name + "_INPUT_MISSING")

        result = json.loads(saved.read_text(encoding="utf-8"))
        records = int(result["records"])

        check(path.stat().st_size == records * 64,
              name + "_TRACE_SIZE")

        for phase in range(1, 5):
            start = records * (phase - 1) // 4
            ref = reference[(name, phase)]

            check(start == int(ref["start_record"]),
                  name + "_START_MISMATCH")

            measured = inspect_window(path, start)

            check(
                measured["unique_lines"] ==
                int(ref["unique_64b_lines"]),
                name + "_UNIQUE_LINE_MISMATCH"
            )

            check(
                measured["address_slots"] ==
                int(ref["memory_address_slots"]),
                name + "_ACCESS_COUNT_MISMATCH"
            )

            check(
                math.isclose(
                    measured["raw_recent_reuse"],
                    float(ref["recent_1024_line_reuse_fraction"]),
                    rel_tol=0, abs_tol=1e-12
                ),
                name + "_REUSE_INDEPENDENT_CHECK_FAILED"
            )

            check(
                math.isclose(
                    measured["hot10_fraction"],
                    float(ref["top10_line_access_fraction"]),
                    rel_tol=0, abs_tol=1e-12
                ),
                name + "_HOT10_CHECK_FAILED"
            )

            check(measured["retained_accesses"] > 0,
                  name + "_NO_RETAINED_ACCESSES")

            row = {
                "workload": name,
                "phase": phase,
                **measured
            }
            rows.append(row)

            print(
                "WINDOW_PASS={} PHASE={} HOT10_PCT={:.3f} "
                "RAW_REUSE_PCT={:.3f} FILTERED_REUSE_PCT={:.3f} "
                "RETAINED_ACCESSES={}".format(
                    name, phase,
                    measured["hot10_fraction"] * 100,
                    measured["raw_recent_reuse"] * 100,
                    measured["hot10_removed_recent_reuse"] * 100,
                    measured["retained_accesses"]
                ), flush=True
            )

    check(len(rows) == 52, "WINDOW_COUNT")

    with (ROOT / "CHAR011_WINDOWS.csv").open(
        "x", newline="", encoding="utf-8"
    ) as f:
        writer = csv.DictWriter(f, fieldnames=list(rows[0]))
        writer.writeheader()
        writer.writerows(rows)

    write_svg(rows)

    audit = {
        "status": "PASS",
        "workloads": 13,
        "windows": 52,
        "independent_reuse_crosschecks": 52,
        "hotline_definition": "10 most frequent 64-byte lines within each window",
        "filtered_history": 1024,
        "filtered_history_scope": "retained address stream only",
        "source_destination_order": "destination then source slots",
        "full_trace_claim": False,
        "limitations": [
            "Filtering changes the reference stream and history spacing",
            "Most frequent lines cannot automatically be identified as stack traffic",
            "This is not an LRU cache simulation or cache hit/miss measurement",
            "Four windows are observations, not independent workload replicates",
            "Memory-access sizes and physical addresses are unavailable"
        ]
    }

    with (ROOT / "CHAR011_AUDIT.json").open(
        "x", encoding="utf-8"
    ) as f:
        json.dump(audit, f, indent=2)
        f.write("\n")

    print("=== CHAR-011 VERDICT ===")
    print("WINDOWS=52")
    print("INDEPENDENT_REUSE_CROSSCHECKS=52")
    print("HOTLINE_SENSITIVITY=COMPLETE")
    print("CHAR011_STATUS=PASS")

    for filename in (
        "CHAR011_WINDOWS.csv",
        "CHAR011_AUDIT.json",
        "CHAR011_REUSE_SENSITIVITY.svg"
    ):
        path = ROOT / filename
        print(
            "OUTPUT={} SHA256={}".format(
                filename, hashlib.sha256(path.read_bytes()).hexdigest()
            )
        )

    return 0

if __name__ == "__main__":
    try:
        sys.exit(main())
    except Exception as exc:
        print("CHAR011_STATUS=FAIL")
        print("ERROR_TYPE={}".format(type(exc).__name__))
        print("ERROR_MESSAGE={}".format(exc))
        print("AUTOMATIC_RETRY=NO")
        sys.exit(2)
