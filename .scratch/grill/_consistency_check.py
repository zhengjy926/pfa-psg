#!/usr/bin/env python
# -*- coding: utf-8 -*-
"""Ancillary consistency checks for the grill review.
1) PRM defined in params.md vs referenced elsewhere.
2) Anchor existence for every relative markdown link with a fragment.
"""
import re
import unicodedata
from pathlib import Path

ROOT = Path(r"F:\pfa-psg")
DOCS = ROOT / "docs"

# ---------- 1. PRM defined vs referenced ----------
params_text = (DOCS / "params.md").read_text(encoding="utf-8")
defined = set(re.findall(r"^\|\s*(PRM-\d{3})\s*\|", params_text, re.M))
referenced = set()
for p in list(DOCS.rglob("*.md")) + [ROOT / "GLOSSARY.md"]:
    if p == DOCS / "params.md":
        continue
    referenced |= set(re.findall(r"PRM-\d{3}", p.read_text(encoding="utf-8")))
print("defined:", len(defined))
print("unreferenced:", sorted(defined - referenced))
print("unknown (referenced but not defined):", sorted(referenced - defined))

# numbering gaps in params.md
nums = sorted(int(m[4:7]) for m in defined)
gaps = []
for band in [(1, 32), (101, 119), (201, 214), (301, 330), (401, 410), (501, 511), (601, 607), (701, 740)]:
    expect = set(range(band[0], band[1] + 1))
    have = {n for n in nums if band[0] <= n <= band[1]}
    gaps += [(band, sorted(expect - have))]
print("numbering gaps (band, missing):", gaps)

# ---------- 2. anchor existence ----------
def slug(text: str) -> str:
    t = unicodedata.normalize("NFC", text.strip().lower())
    out = []
    for ch in t:
        if ch.isalnum() or ch in "-_":
            out.append(ch)
        elif ch == " ":
            out.append("-")
    return "".join(out)

anchors = {}
for p in list(DOCS.rglob("*.md")) + [ROOT / "GLOSSARY.md"]:
    seen = {}
    for line in p.read_text(encoding="utf-8").splitlines():
        m = re.match(r"^(#{1,6})\s+(.*)$", line)
        if not m:
            continue
        base = slug(m.group(2))
        if not base:
            continue
        if base in seen:
            seen[base] += 1
            base2 = f"{base}-{seen[base]}"
        else:
            seen[base] = 0
            base2 = base
        anchors.setdefault(base2, []).append((p, m.group(2).strip()))

bad = []
checked = 0
for p in list(DOCS.rglob("*.md")) + [ROOT / "GLOSSARY.md"]:
    text = p.read_text(encoding="utf-8")
    for m in re.finditer(r"\]\(([^()\s]+)\)", text):
        target = m.group(1)
        if target.startswith(("http://", "https://", "mailto:")) or "#" not in target:
            continue
        path_part, frag = target.split("#", 1)
        tp = (p.parent / path_part).resolve()
        if not tp.exists() or not frag:
            continue
        checked += 1
        if frag not in anchors:
            bad.append((str(p.relative_to(ROOT)), target))

print("anchors checked:", checked)
if bad:
    print("BROKEN ANCHORS:")
    for src, tgt in sorted(bad):
        print(f"  {src} -> {tgt}")
else:
    print("all anchors OK")
