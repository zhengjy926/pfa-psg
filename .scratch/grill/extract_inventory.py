# -*- coding: utf-8 -*-
"""Extract PRM / command-code / SRS-ID / signal-name occurrences from the five spec docs."""
import re, os, sys

FILES = [
    (r"docs\spec\product-cardiac-pfa.md", "product"),
    (r"docs\spec\board-pulse-gen.md", "board"),
    (r"docs\spec\ecsp-general.md", "ecsp"),
    (r"docs\srs\pulse-gen-firmware.md", "srs"),
    (r".scratch\psg-firmware\spec.md", "scratch"),
]

PRM_RE = re.compile(r"PRM-(\d{3})")
CMD_RE = re.compile(r"\b0x([0-9A-Fa-f]{2})\b")
SRS_RE = re.compile(r"\b([FITEN])-(\d{3})\b")
SIG_RE = re.compile(r"\b(P[ABCDEFG]\d{1,2}|USART[0-9]|HRTIM[A-Z0-9_]*|DAC[0-9]|ADC[0-9]|COMP[0-9]|FLT[0-9]|EXTI[0-9_]*|IWDG|SWD|LSI|SYSCLK|HSE|PLL[MPR]|VCO|SysTick|fHRTIM|CMP[0-9]|TA[0-9]|TB)\b")

# command codes we treat as commands (avoid frame-tail 0x0D etc.)
CMD_SET = {"01","02","03","04","05","06","07","08","09","0a","0b","0c","0d","0e","0f","10","11","12",
           "30","31","32","33","34","35","36","37","38","39","3a","3b","3c","3d","3e","3f","40","41","42","43","44","45"}
def is_cmd(h):
    return h.lower() in CMD_SET and h.lower() != "0d"

def scan(path):
    rows = {"prm": {}, "cmd": {}, "srs": {}, "sig": {}}
    with open(path, "r", encoding="utf-8") as f:
        for i, line in enumerate(f, 1):
            for m in PRM_RE.finditer(line):
                rows["prm"].setdefault(m.group(1), []).append(i)
            for m in CMD_RE.finditer(line):
                h = m.group(1)
                if is_cmd(h):
                    rows["cmd"].setdefault(h.upper(), []).append(i)
            for m in SRS_RE.finditer(line):
                rows["srs"].setdefault(f"{m.group(1)}-{m.group(2)}", []).append(i)
            for m in SIG_RE.finditer(line):
                rows["sig"].setdefault(m.group(1), []).append(i)
    return rows

out = []
out.append("# Inventory raw\n")
for path, tag in FILES:
    rows = scan(path)
    out.append(f"\n## {tag} :: {path}\n")
    out.append("### PRM\n")
    for k in sorted(rows["prm"]):
        lines = ",".join(str(x) for x in rows["prm"][k])
        out.append(f"- PRM-{k}: {lines}")
    out.append("### CMD\n")
    for k in sorted(rows["cmd"]):
        lines = ",".join(str(x) for x in rows["cmd"][k])
        out.append(f"- 0x{k}: {lines}")
    out.append("### SRS\n")
    for k in sorted(rows["srs"]):
        lines = ",".join(str(x) for x in rows["srs"][k])
        out.append(f"- {k}: {lines}")
    out.append("### SIG\n")
    for k in sorted(rows["sig"]):
        lines = ",".join(str(x) for x in rows["sig"][k])
        out.append(f"- {k}: {lines}")

with open(r".scratch\grill\inventory-raw.md", "w", encoding="utf-8") as f:
    f.write("\n".join(out))
print("written", sum(len(scan(p)[0]) for p, _ in FILES), "rows total")
