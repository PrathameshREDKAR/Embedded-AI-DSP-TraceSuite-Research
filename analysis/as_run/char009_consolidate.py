import csv
import json
import hashlib
import math
import sys
from pathlib import Path
from xml.sax.saxutils import escape

ROOT = Path(r"D:\Embedded_AI_DSP_TraceSuite\ACCML_CHAR_20261109_V1")

NAMES = [
    "FFT", "FIR", "CONV", "MATMUL", "IIR", "DCT",
    "DWT", "LMS", "POLYPHASE", "STFT", "MFCC",
    "BEAMFORM", "AEC_NLMS"
]

CP26_RAW_HASH = (
    "e79a000aeb53fc22bbe74d9023544b2cac22"
    "ff1a3219a902d90cab506a4b1e3c"
)

def check(condition, description):
    if not condition:
        raise ValueError(description)

def read_results(name):
    filename = (
        "AEC_NLMS_provisional_full_v1.json"
        if name == "AEC_NLMS"
        else name + "_full_v1.json"
    )
    with (ROOT / filename).open("r", encoding="utf-8") as f:
        r = json.load(f)

    check(r["workload"] == name, name + "_NAME_MISMATCH")
    check(r["records"] > 0, name + "_EMPTY")
    check(r["record_bytes"] == 64, name + "_BAD_ABI")
    check(r["cache_line_bytes"] == 64, name + "_BAD_LINE_SIZE")
    check(r["zero_ip"] == 0, name + "_ZERO_IP")
    check(r["invalid_flags"] == 0, name + "_INVALID_FLAGS")
    check(
        r["status"] == (
            "COMPLETE_PROVISIONAL"
            if name == "AEC_NLMS" else "COMPLETE"
        ),
        name + "_STATUS"
    )

    if name == "AEC_NLMS":
        check(r["records"] == 104556305, "CP26_RECORDS")
        check(r["raw_sha256"] == CP26_RAW_HASH, "CP26_RAW_HASH")
        check(
            r["compressed_sha256"] ==
            "92791ee4c4ffeb357f8302ecd8143cc79e42c73c4bf6d98ff6563d045c5330b7",
            "CP26_ARCHIVE_HASH"
        )
    else:
        check(r["records"] == r["expected_records"],
              name + "_RECORD_COUNT")

    loads = r["load_slots"]
    stores = r["store_slots"]
    total = loads + stores
    records = r["records"]

    check(loads > 0 and stores > 0, name + "_NO_ACCESSES")
    check(total == r["memory_address_slots"], name + "_TOTAL")
    check(0 <= r["branches"] <= records, name + "_BRANCHES")
    check(r["load_transitions"] == loads - 1, name + "_LOAD_TRANSITIONS")
    check(r["store_transitions"] == stores - 1, name + "_STORE_TRANSITIONS")

    for actual, expected, label in (
        (r["load_fraction"], loads / total, "LOAD_FRACTION"),
        (r["branch_fraction"], r["branches"] / records, "BRANCH_FRACTION"),
        (r["load_same_line_fraction"],
         r["load_same_line"] / (loads - 1), "SAME_LINE")
    ):
        check(
            math.isfinite(actual) and
            math.isclose(actual, expected, rel_tol=0, abs_tol=1e-12),
            name + "_" + label
        )

    for i in range(1, 5):
        check(
            r["window{}_unique_lines".format(i)] > 0,
            name + "_WINDOW"
        )

    return {
        "workload": name,
        "records": records,
        "load_slots": loads,
        "store_slots": stores,
        "load_fraction": r["load_fraction"],
        "branch_fraction": r["branch_fraction"],
        "memory_slots_per_instruction": total / records,
        "load_same_line_fraction": r["load_same_line_fraction"],
        "store_same_line_fraction": r["store_same_line_fraction"],
        "load_plus8_fraction": r["load_plus8_fraction"],
        "store_plus8_fraction": r["store_plus8_fraction"],
        **{
            "window{}_unique_lines".format(i):
            r["window{}_unique_lines".format(i)]
            for i in range(1, 5)
        },
        "raw_sha256": r["raw_sha256"],
        "trace_accepted": True,
        "provenance_note": (
            "Accepted canonical trace selected from 1x Pin sizing pilot"
            if name == "AEC_NLMS" else
            "Final trace candidate from closed workload"
        )
    }

def make_figure(rows, field, title, filename):
    width = 1000
    height = 690
    x0 = 155
    y0 = 90
    bar_max = 670

    out = [
        '<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 1000 690">',
        '<rect width="1000" height="690" fill="white"/>',
        '<text x="155" y="40" font-family="Arial" font-size="22" font-weight="bold">{}</text>'.format(escape(title))
    ]

    for tick in range(0, 101, 20):
        x = x0 + bar_max * tick / 100
        out.append(
            '<line x1="{:.2f}" y1="75" x2="{:.2f}" y2="625" stroke="#ddd"/>'.format(x, x)
        )
        out.append(
            '<text x="{:.2f}" y="649" font-family="Arial" font-size="12" text-anchor="middle">{}%</text>'.format(x, tick)
        )

    for i, r in enumerate(rows):
        y = y0 + i * 40
        v = 100 * r[field]
        check(0 <= v <= 100, "FIGURE_VALUE_OUT_OF_RANGE")
        w = bar_max * v / 100
        out.append(
            '<text x="143" y="{}" font-family="Arial" font-size="13" text-anchor="end">{}</text>'.format(
                y + 16, escape(r["workload"])
            )
        )
        out.append(
            '<rect x="155" y="{}" width="{:.2f}" height="23" fill="#405365"/>'.format(y, w)
        )
        out.append(
            '<text x="{:.2f}" y="{}" font-family="Arial" font-size="12">{:.2f}%</text>'.format(
                x0 + w + 7, y + 16, v
            )
        )

    out.append('</svg>')

    with (ROOT / filename).open("x", encoding="utf-8") as f:
        f.write("\n".join(out) + "\n")

def main():
    previous = json.loads(
        (ROOT / "CHAR005_AUDIT.json").read_text(encoding="utf-8")
    )
    check(previous["status"] == "PASS" and previous["workloads"] == 12,
          "PRIOR_AUDIT_FAILED")

    rows = [read_results(name) for name in NAMES]
    check(len(rows) == 13, "COHORT_SIZE")

    with (ROOT / "CHAR009_ALL13.csv").open(
        "x", encoding="utf-8", newline=""
    ) as f:
        writer = csv.DictWriter(f, fieldnames=list(rows[0]))
        writer.writeheader()
        writer.writerows(rows)

    make_figure(
        rows, "load_fraction",
        "Load-address fraction across 13 DSP workloads",
        "CHAR009_LOAD_FRACTION.svg"
    )
    make_figure(
        rows, "branch_fraction",
        "Dynamic branch instruction frequency",
        "CHAR009_BRANCH_FREQUENCY.svg"
    )
    make_figure(
        rows, "load_same_line_fraction",
        "Consecutive load addresses in the same 64-byte line",
        "CHAR009_SPATIAL_LOCALITY.svg"
    )

    audit = {
        "status": "PASS",
        "accepted_workloads": 13,
        "records_total": sum(r["records"] for r in rows),
        "prior_main_cohort_audit": "CHAR005_PASS",
        "cp26_frozen_closure_attestation_verified_by_powershell": True,
        "cp26_original_trace_role": "1x Pin sizing pilot, subsequently accepted as canonical final",
        "analysis_type": "consolidation_and_arithmetic_audit",
        "new_trace_scan": False,
        "independent_raw_recomputation": False,
        "limitations": [
            "Address-slot counts are not distinct memory instructions",
            "Adjacent-address same-line fraction is not cache hit rate",
            "Working-set windows cover 100000 instructions each, not entire traces",
            "Exact access sizes are not present in trace ABI",
            "CP26 accepted from prior 1x sizing pilot"
        ]
    }
    with (ROOT / "CHAR009_AUDIT.json").open(
        "x", encoding="utf-8"
    ) as f:
        json.dump(audit, f, indent=2)
        f.write("\n")

    print("=== CHAR-009 RESULTS ===")
    for r in rows:
        print(
            "WORKLOAD={} RECORDS={} LOAD_PCT={:.3f} "
            "BRANCH_PCT={:.3f} SAME_LINE_PCT={:.3f}".format(
                r["workload"], r["records"],
                100*r["load_fraction"],
                100*r["branch_fraction"],
                100*r["load_same_line_fraction"]
            )
        )

    print("ACCEPTED_WORKLOADS=13")
    print("TOTAL_RECORDS={}".format(audit["records_total"]))
    print("CSV_CREATED=1")
    print("FIGURES_CREATED=3")
    print("AUDIT_CREATED=1")

    for filename in (
        "CHAR009_ALL13.csv",
        "CHAR009_AUDIT.json",
        "CHAR009_LOAD_FRACTION.svg",
        "CHAR009_BRANCH_FREQUENCY.svg",
        "CHAR009_SPATIAL_LOCALITY.svg"
    ):
        p = ROOT / filename
        print(
            "OUTPUT_SHA256={} {}".format(
                filename, hashlib.sha256(p.read_bytes()).hexdigest()
            )
        )

    print("CHAR009_STATUS=PASS")
    return 0

if __name__ == "__main__":
    try:
        sys.exit(main())
    except Exception as exc:
        print("CHAR009_STATUS=FAIL")
        print("ERROR={}".format(exc))
        print("DO_NOT_RERUN=YES")
        sys.exit(2)
