import csv
import hashlib
import json
import math
import statistics
import sys
from collections import defaultdict
from pathlib import Path

ROOT = Path(r"D:\Embedded_AI_DSP_TraceSuite\ACCML_CHAR_20261109_V1")

EXPECTED = [
    "FFT", "FIR", "CONV", "MATMUL", "IIR", "DCT",
    "DWT", "LMS", "POLYPHASE", "STFT", "MFCC",
    "BEAMFORM", "AEC_NLMS"
]

def require(ok, message):
    if not ok:
        raise ValueError(message)

def read_csv(name):
    with (ROOT / name).open("r", newline="", encoding="utf-8") as f:
        return list(csv.DictReader(f))

def mean(values):
    return statistics.mean(values)

def main():
    full = read_csv("CHAR009_ALL13.csv")
    windows = read_csv("CHAR010_WINDOWS.csv")
    sensitivity = read_csv("CHAR011_WINDOWS.csv")

    require(len(full) == 13, "FULL_TRACE_ROW_COUNT")
    require(len(windows) == 52, "TEMPORAL_WINDOW_ROW_COUNT")
    require(len(sensitivity) == 52, "SENSITIVITY_ROW_COUNT")
    require([r["workload"] for r in full] == EXPECTED,
            "WORKLOAD_ORDER_MISMATCH")

    by_temporal = defaultdict(dict)
    by_sensitivity = defaultdict(dict)

    for r in windows:
        key = (r["workload"], int(r["phase"]))
        require(key[1] in (1, 2, 3, 4), "TEMPORAL_PHASE")
        require(key[1] not in by_temporal[key[0]], "DUPLICATE_TEMPORAL")
        by_temporal[key[0]][key[1]] = r

    for r in sensitivity:
        key = (r["workload"], int(r["phase"]))
        require(key[1] in (1, 2, 3, 4), "SENSITIVITY_PHASE")
        require(key[1] not in by_sensitivity[key[0]], "DUPLICATE_SENSITIVITY")
        by_sensitivity[key[0]][key[1]] = r

    require(set(by_temporal) == set(EXPECTED), "TEMPORAL_COHORT")
    require(set(by_sensitivity) == set(EXPECTED), "SENSITIVITY_COHORT")

    result = []

    for r in full:
        name = r["workload"]
        require(set(by_temporal[name]) == {1,2,3,4}, name + "_TEMP_PHASES")
        require(set(by_sensitivity[name]) == {1,2,3,4}, name + "_SENS_PHASES")

        t = [by_temporal[name][p] for p in range(1,5)]
        s = [by_sensitivity[name][p] for p in range(1,5)]

        unique = [int(x["unique_64b_lines"]) for x in t]
        raw = [float(x["raw_recent_reuse"]) for x in s]
        filtered = [float(x["hot10_removed_recent_reuse"]) for x in s]
        hot = [float(x["hot10_fraction"]) for x in s]

        for i in range(4):
            require(unique[i] == int(s[i]["unique_lines"]),
                    name + "_UNIQUE_LINE_CROSSCHECK")
            require(int(t[i]["memory_address_slots"]) ==
                    int(s[i]["address_slots"]),
                    name + "_ADDRESS_COUNT_CROSSCHECK")
            require(math.isclose(
                float(t[i]["recent_1024_line_reuse_fraction"]),
                raw[i], rel_tol=0, abs_tol=1e-12
            ), name + "_REUSE_CROSSCHECK")
            require(math.isclose(
                float(t[i]["top10_line_access_fraction"]),
                hot[i], rel_tol=0, abs_tol=1e-12
            ), name + "_HOT10_CROSSCHECK")
            require(
                0 <= filtered[i] <= 1 and 0 <= raw[i] <= 1,
                name + "_INVALID_FRACTION"
            )

        result.append({
            "workload": name,
            "full_records": int(r["records"]),
            "full_load_pct": 100 * float(r["load_fraction"]),
            "full_branch_pct": 100 * float(r["branch_fraction"]),
            "full_load_same_line_pct":
                100 * float(r["load_same_line_fraction"]),
            "mean_hot10_pct": 100 * mean(hot),
            "mean_raw_recent_reuse_pct": 100 * mean(raw),
            "mean_filtered_recent_reuse_pct": 100 * mean(filtered),
            "filtered_minus_raw_pp": 100 * (mean(filtered)-mean(raw)),
            "min_window_unique_lines": min(unique),
            "max_window_unique_lines": max(unique),
            "window_unique_line_ratio": max(unique)/min(unique),
            "window_unique_lines": "/".join(map(str, unique))
        })

    require(sum(x["full_records"] for x in result) == 1274683876,
            "TOTAL_RECORD_MISMATCH")

    output = ROOT / "CHAR012_WORKLOAD_SUMMARY.csv"
    with output.open("x", newline="", encoding="utf-8") as f:
        writer = csv.DictWriter(f, fieldnames=list(result[0]))
        writer.writeheader()
        writer.writerows(result)

    sorted_phase = sorted(
        result, key=lambda x: x["window_unique_line_ratio"], reverse=True
    )
    sorted_sensitivity = sorted(
        result, key=lambda x: x["filtered_minus_raw_pp"]
    )

    lines = [
        "ACCML CHAR-012 SCIENTIFIC FINDINGS",
        "================================",
        "",
        "COHORT: 13 accepted frozen traces",
        "FULL DYNAMIC INSTRUCTION RECORDS: 1,274,683,876",
        "FULL-TRACE ANALYSIS: CHAR-004 + CHAR-006",
        "CONSOLIDATION: CHAR-009",
        "TEMPORAL SAMPLING: 4 x 100,000 instructions per workload",
        "HOT-LINE SENSITIVITY: top 10 lines excluded per window",
        "",
        "FINDING 1: FULL-TRACE SPATIAL LOCALITY",
        "The consecutive-load same-64-byte-line fraction spans:",
        "Minimum: {} {:.3f}%".format(
            min(result, key=lambda x:x["full_load_same_line_pct"])["workload"],
            min(x["full_load_same_line_pct"] for x in result)),
        "Maximum: {} {:.3f}%".format(
            max(result, key=lambda x:x["full_load_same_line_pct"])["workload"],
            max(x["full_load_same_line_pct"] for x in result)),
        "",
        "FINDING 2: HOT-LINE SENSITIVITY",
        "Values are four-window arithmetic means per workload.",
    ]

    for x in sorted_sensitivity:
        lines.append(
            "{}: raw={:.3f}% filtered={:.3f}% "
            "delta={:+.3f} percentage points; hot10={:.3f}%".format(
                x["workload"], x["mean_raw_recent_reuse_pct"],
                x["mean_filtered_recent_reuse_pct"],
                x["filtered_minus_raw_pp"], x["mean_hot10_pct"]
            )
        )

    lines.extend([
        "",
        "FINDING 3: SAMPLED WINDOW FOOTPRINT VARIATION",
        "Sorted by maximum/minimum unique-line count:",
    ])

    for x in sorted_phase:
        lines.append(
            "{}: windows={} ratio={:.3f}".format(
                x["workload"], x["window_unique_lines"],
                x["window_unique_line_ratio"]
            )
        )

    lines.extend([
        "",
        "INTERPRETATION RULES",
        "- 64 bytes is the assumed cache-line size, not access size.",
        "- Recent reuse is not cache hit rate or LRU stack distance.",
        "- Filtered reuse changes the stream and the 1024-access history.",
        "- Hot addresses are not proven to be stack addresses.",
        "- The four sample windows are not independent trials.",
        "- Full-trace metrics must not be confused with window metrics.",
        "- Phase changes remain hypotheses pending execution-stage mapping.",
        "- No prefetching speedup is established by this characterization.",
        "- CP26's accepted canonical trace originated as a Pin sizing pilot.",
        "",
        "CP27: ON HOLD",
        "NO TRACE GENERATION, SIMULATION, OR SOURCE MODIFICATION.",
    ])

    report = ROOT / "CHAR012_SCIENTIFIC_FINDINGS.txt"
    with report.open("x", encoding="utf-8") as f:
        f.write("\n".join(lines) + "\n")

    audit = {
        "status": "PASS",
        "workloads": 13,
        "full_records": 1274683876,
        "sampled_windows": 52,
        "temporal_crosschecks": 52,
        "new_trace_scan": False,
        "no_source_modification": True,
        "cp27_on_hold": True,
        "statistical_unit": "workload; windows are descriptive samples",
        "aggregation": "unweighted arithmetic mean of four windows"
    }

    audit_path = ROOT / "CHAR012_AUDIT.json"
    with audit_path.open("x", encoding="utf-8") as f:
        json.dump(audit, f, indent=2)
        f.write("\n")

    print("=== CHAR-012 SUMMARY ===")
    print("WORKLOADS=13")
    print("TOTAL_RECORDS=1274683876")
    print("WINDOW_CROSSCHECKS=52")
    print("FULL_TRACE_RECOMPUTATION=NO")

    for x in result:
        print(
            "WORKLOAD={} RAW_REUSE={:.3f} FILTERED_REUSE={:.3f} "
            "DELTA_PP={:+.3f} HOT10={:.3f} WINDOW_RATIO={:.3f}".format(
                x["workload"],
                x["mean_raw_recent_reuse_pct"],
                x["mean_filtered_recent_reuse_pct"],
                x["filtered_minus_raw_pp"],
                x["mean_hot10_pct"],
                x["window_unique_line_ratio"]
            )
        )

    for path in (output, report, audit_path):
        print(
            "OUTPUT={} SHA256={}".format(
                path.name, hashlib.sha256(path.read_bytes()).hexdigest()
            )
        )

    print("CHAR012_STATUS=PASS")
    return 0

if __name__ == "__main__":
    try:
        sys.exit(main())
    except Exception as exc:
        print("CHAR012_STATUS=FAIL")
        print("ERROR_TYPE={}".format(type(exc).__name__))
        print("ERROR={}".format(exc))
        print("AUTOMATIC_RETRY=NO")
        sys.exit(2)
