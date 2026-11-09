import csv
import hashlib
import json
import math
import sys
from pathlib import Path
from xml.sax.saxutils import escape

ROOT = Path(r"D:\Embedded_AI_DSP_TraceSuite\ACCML_CHAR_20261109_V1")

WORKLOADS = [
    ("FFT", 69607705, "19d1fb8678e87db07c1e2fb37658a3bbf58175001da32fd1c59422cf6dc9f791"),
    ("FIR", 68105642, "0d90a089863922c3fe4e33b3677e9c718755f30158646d82685f1508ccefae91"),
    ("CONV", 65426935, "cdc45bc1d5fc8dc927386f886ae884728863c9f4e00c4908501cd940596bde82"),
    ("MATMUL", 101321253, "e833bb132fad3b4642aaf9ae0b577ff81a10c3cfe1e1936125e7b72fa8072c09"),
    ("IIR", 62390447, None),
    ("DCT", 63009469, "c056e9e97d5566ec238f59aefc50b813fb9c76f031ebfed9562a9605279cfa12"),
    ("DWT", 64254895, "a70dd6fab2796ff708047bf360bcca007ff7757d543863d8f8d0410dea3c60fc"),
    ("LMS", 64722031, None),
    ("POLYPHASE", 69065955, None),
    ("STFT", 80714506, None),
    ("MFCC", 114614807, None),
    ("BEAMFORM", 346893926, None),
]

def require(condition, message):
    if not condition:
        raise ValueError(message)

def finite_fraction(value, label):
    require(isinstance(value, (int, float)), label + "_NOT_NUMERIC")
    require(math.isfinite(value) and 0 <= value <= 1, label + "_INVALID")
    return float(value)

def svg_bars(rows, field, title, subtitle, filename):
    width, height = 1050, 660
    left, top = 170, 105
    plot_width = 700
    row_height = 39

    elements = [
        '<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 1050 660">',
        '<rect width="1050" height="660" fill="white"/>',
        '<text x="170" y="43" font-size="23" font-family="Arial" font-weight="bold">{}</text>'.format(escape(title)),
        '<text x="170" y="69" font-size="14" font-family="Arial" fill="#555">{}</text>'.format(escape(subtitle))
    ]

    for tick in range(0, 101, 20):
        x = left + plot_width * tick / 100
        elements.append(
            '<line x1="{:.1f}" y1="90" x2="{:.1f}" y2="590" stroke="#ddd"/>'.format(x, x)
        )
        elements.append(
            '<text x="{:.1f}" y="610" font-size="13" font-family="Arial" text-anchor="middle">{}</text>'.format(x, tick)
        )

    for i, row in enumerate(rows):
        y = top + i * row_height
        value = finite_fraction(row[field], field)
        bar_width = value * plot_width

        elements.append(
            '<text x="155" y="{}" font-size="14" font-family="Arial" text-anchor="end">{}</text>'.format(
                y + 18, escape(row["workload"])
            )
        )
        elements.append(
            '<rect x="170" y="{}" width="{:.2f}" height="23" fill="#33465a"/>'.format(
                y, bar_width
            )
        )
        elements.append(
            '<text x="{:.2f}" y="{}" font-size="13" font-family="Arial">{:.2f}%</text>'.format(
                left + bar_width + 7, y + 17, value * 100
            )
        )

    elements.append(
        '<text x="520" y="640" font-size="14" font-family="Arial" text-anchor="middle">Percentage (%)</text>'
    )
    elements.append('</svg>')

    path = ROOT / filename
    with path.open("x", encoding="utf-8") as f:
        f.write("\n".join(elements) + "\n")

def main():
    rows = []
    checks = []

    for name, expected_records, known_hash in WORKLOADS:
        path = ROOT / (name + "_full_v1.json")
        require(path.is_file(), name + "_RESULT_MISSING")

        with path.open("r", encoding="utf-8") as f:
            r = json.load(f)

        require(r["workload"] == name, name + "_NAME")
        require(r["status"] == "COMPLETE", name + "_STATUS")
        require(r["records"] == expected_records, name + "_RECORD_COUNT")
        require(r["expected_records"] == expected_records, name + "_EXPECTED_COUNT")
        require(r["record_bytes"] == 64, name + "_RECORD_ABI")
        require(r["cache_line_bytes"] == 64, name + "_CACHE_LINE_SIZE")
        require(r["invalid_flags"] == 0, name + "_INVALID_FLAGS")
        require(r["zero_ip"] == 0, name + "_ZERO_IP")

        digest = r["raw_sha256"]
        require(len(digest) == 64 and all(c in "0123456789abcdef" for c in digest),
                name + "_RAW_DIGEST_FORMAT")

        if known_hash is not None:
            require(digest == known_hash, name + "_HISTORICAL_RAW_HASH")

        load = r["load_slots"]
        store = r["store_slots"]
        total = load + store

        require(load > 0 and store > 0, name + "_EMPTY_MEMORY_STREAM")
        require(r["memory_address_slots"] == total, name + "_MEMORY_TOTAL")
        require(0 <= r["branches"] <= expected_records, name + "_BRANCH_COUNT")
        require(0 <= r["instructions_with_loads"] <= expected_records, name + "_LOAD_INSTRUCTION_COUNT")
        require(0 <= r["instructions_with_stores"] <= expected_records, name + "_STORE_INSTRUCTION_COUNT")

        require(
            math.isclose(r["load_fraction"], load / total, rel_tol=0, abs_tol=1e-12),
            name + "_LOAD_FRACTION"
        )

        require(
            math.isclose(r["branch_fraction"], r["branches"] / expected_records,
                         rel_tol=0, abs_tol=1e-12),
            name + "_BRANCH_FRACTION"
        )

        require(
            math.isclose(r["memory_slots_per_instruction"], total / expected_records,
                         rel_tol=0, abs_tol=1e-12),
            name + "_MEMORY_DENSITY"
        )

        for kind, slots in (("load", load), ("store", store)):
            prefix = kind + "_"
            transitions = r[prefix + "transitions"]

            require(transitions == slots - 1, name + "_" + kind + "_TRANSITIONS")

            categories = [
                "zero", "plus8", "minus8", "plus64", "minus64"
            ]

            for category in categories:
                value = r[prefix + category]
                require(0 <= value <= transitions,
                        name + "_" + kind + "_" + category)

            require(
                sum(r[prefix + c] for c in categories) <= transitions,
                name + "_" + kind + "_STRIDE_SUM"
            )

            require(
                0 <= r[prefix + "same_line"] <= transitions,
                name + "_" + kind + "_SAME_LINE_COUNT"
            )

            finite_fraction(
                r[prefix + "same_line_fraction"],
                name + "_" + kind + "_SAME_LINE_FRACTION"
            )

            require(
                math.isclose(
                    r[prefix + "same_line_fraction"],
                    r[prefix + "same_line"] / transitions,
                    rel_tol=0, abs_tol=1e-12
                ),
                name + "_" + kind + "_SAME_LINE_RATIO"
            )

        for index in range(1, 5):
            lines = r["window{}_unique_lines".format(index)]
            require(isinstance(lines, int) and lines > 0,
                    name + "_WINDOW{}_INVALID".format(index))

        rows.append({
            "workload": name,
            "records": expected_records,
            "load_slots": load,
            "store_slots": store,
            "load_fraction": r["load_fraction"],
            "branch_fraction": r["branch_fraction"],
            "memory_slots_per_instruction": r["memory_slots_per_instruction"],
            "load_same_line_fraction": r["load_same_line_fraction"],
            "store_same_line_fraction": r["store_same_line_fraction"],
            "load_plus8_fraction": r["load_plus8_fraction"],
            "store_plus8_fraction": r["store_plus8_fraction"],
            "window1_unique_lines": r["window1_unique_lines"],
            "window2_unique_lines": r["window2_unique_lines"],
            "window3_unique_lines": r["window3_unique_lines"],
            "window4_unique_lines": r["window4_unique_lines"],
            "raw_sha256": digest,
            "historical_raw_hash_verified": known_hash is not None,
        })

        checks.append(name + "_AUDIT_PASS")

    require(len(rows) == 12, "WORKLOAD_COUNT")

    csv_path = ROOT / "CHAR005_SUMMARY.csv"
    with csv_path.open("x", newline="", encoding="utf-8") as f:
        writer = csv.DictWriter(f, fieldnames=list(rows[0].keys()))
        writer.writeheader()
        writer.writerows(rows)

    svg_bars(
        rows, "load_fraction",
        "Load-address fraction across 12 traces",
        "Full-trace source-memory slots / all nonzero memory slots",
        "CHAR005_LOAD_FRACTION.svg"
    )

    svg_bars(
        rows, "branch_fraction",
        "Dynamic branch frequency across 12 traces",
        "Full-trace branch instructions / dynamic instructions",
        "CHAR005_BRANCH_FREQUENCY.svg"
    )

    svg_bars(
        rows, "load_same_line_fraction",
        "Consecutive load accesses in the same cache line",
        "Adjacent load-address transitions; 64-byte lines",
        "CHAR005_CACHE_LINE_LOCALITY.svg"
    )

    audit = {
        "status": "PASS",
        "workloads": len(rows),
        "validation_type": "independent_arithmetic_and_metadata_checks",
        "raw_bytes_recomputed": False,
        "external_reexecution": False,
        "historical_raw_hash_matches": sum(
            int(r["historical_raw_hash_verified"]) for r in rows
        ),
        "checked_workloads": checks,
        "cp26_aec_nlms_excluded": True,
        "cp27_on_hold": True,
        "limitations": [
            "No external recomputation of raw memory statistics in this step",
            "Working-set windows are sampled, not complete-trace footprints",
            "Consecutive same-line fraction is not a cache hit rate",
            "Exact memory-access sizes are unavailable",
            "Later raw hashes await reconciliation with historical manifests"
        ],
    }

    with (ROOT / "CHAR005_AUDIT.json").open("x", encoding="utf-8") as f:
        json.dump(audit, f, indent=2)
        f.write("\n")

    print("=== CHAR-005 AUDITED RESULTS ===")
    for r in rows:
        print(
            "WORKLOAD={} RECORDS={} LOAD_PCT={:.3f} BRANCH_PCT={:.3f} "
            "LOAD_SAME_LINE_PCT={:.3f} FOUR_WINDOWS={},{},{},{}".format(
                r["workload"], r["records"],
                100 * r["load_fraction"],
                100 * r["branch_fraction"],
                100 * r["load_same_line_fraction"],
                r["window1_unique_lines"],
                r["window2_unique_lines"],
                r["window3_unique_lines"],
                r["window4_unique_lines"]
            )
        )

    print("VALIDATED_WORKLOADS=12")
    print("HISTORICAL_RAW_HASH_MATCHES={}".format(
        audit["historical_raw_hash_matches"]
    ))
    print("CSV_CREATED=CHAR005_SUMMARY.csv")
    print("FIGURES_CREATED=3")
    print("AUDIT_FILE_CREATED=CHAR005_AUDIT.json")

    for name in (
        "CHAR005_SUMMARY.csv",
        "CHAR005_AUDIT.json",
        "CHAR005_LOAD_FRACTION.svg",
        "CHAR005_BRANCH_FREQUENCY.svg",
        "CHAR005_CACHE_LINE_LOCALITY.svg"
    ):
        p = ROOT / name
        digest = hashlib.sha256(p.read_bytes()).hexdigest()
        print("OUTPUT={} SHA256={}".format(name, digest))

    print("CHAR005_STATUS=PASS")
    print("NEXT=CP26_PROVISIONAL_CHARACTERIZATION_AND_FINAL_SCIENTIFIC_REVIEW")
    return 0

if __name__ == "__main__":
    try:
        sys.exit(main())
    except Exception as exc:
        print("CHAR005_STATUS=FAIL")
        print("ERROR_TYPE={}".format(type(exc).__name__))
        print("ERROR_MESSAGE={}".format(str(exc)))
        print("PRESERVE_PARTIAL_OUTPUTS=YES")
        sys.exit(2)
