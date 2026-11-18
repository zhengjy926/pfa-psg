#!/usr/bin/env python
# -*- coding: utf-8 -*-
"""Cross-check the agent-readable docs against their registries.

Checks
  1. every PRM-xxx referenced anywhere exists in docs/params.md
  2. no bare [待确认] left without a [待补] target
  3. no leftover [CRC16] / [FA] placeholders from the baseline docs
  4. GLOSSARY term count and colon style
  5. duplicate requirement IDs inside the SRS

Usage: python tools/doc_check.py
"""
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
DOCS = ROOT / "docs"

PARAMS = DOCS / "params.md"
GLOSSARY = ROOT / "GLOSSARY.md"


def load_ids(path: Path, pattern: str):
    return set(re.findall(pattern, path.read_text(encoding="utf-8"), re.M))


def main() -> int:
    prm = load_ids(PARAMS, r"^\|\s*(PRM-\d{3})\s*\|")
    problems = 0

    print(f"registry: {len(prm)} PRM")
    for doc in sorted(list(DOCS.rglob("*.md")) + ([GLOSSARY] if GLOSSARY.exists() else [])):
        text = doc.read_text(encoding="utf-8")
        rel = doc.relative_to(ROOT)
        refs = set(re.findall(r"PRM-\d{3}", text))
        unknown_prm = sorted(refs - prm)
        bare = re.findall(r"\[待确认\](?!\s*Q-\d)", text)
        leftovers = [tok for tok in ("[CRC16]", "[FA]") if tok in text]
        # the documents describe the present, not where they came from
        banned = []
        if re.search(r"Q-\d{3}", text):
            banned.append("Q-nnn reference")
        if "OPEN_QUESTIONS" in text:
            banned.append("OPEN_QUESTIONS reference")
        if "原 §" in text or "原文 §" in text:
            banned.append("baseline section locator")
        if "[未收录]" in text:
            banned.append("[未收录] marker")
        crc = text.count("{{CRC}}")
        flags = []
        if unknown_prm:
            flags.append(f"unknown PRM {unknown_prm}")
        if bare:
            flags.append(f"{len(bare)} bare [待确认]")
        if leftovers:
            flags.append(f"leftover {leftovers}")
        if banned:
            flags.append("banned " + ", ".join(banned))
        if flags:
            problems += len(flags)
        print(
            f"  {str(rel):46s} PRM {len(refs):3d}  CRC {crc:2d}"
            + ("   <-- " + "; ".join(flags) if flags else "")
        )

    if GLOSSARY.exists():
        g = GLOSSARY.read_text(encoding="utf-8")
        terms = re.findall(r"^\*\*(.+?)\*\*:", g, re.M)
        full = len(re.findall(r"^\*\*(.+?)\*\*\s*：", g, re.M))
        dups = {t for t in terms if terms.count(t) > 1}
        print(f"glossary: {len(terms)} terms, {full} full-width colons, dups={sorted(dups)}")
        if full or dups:
            problems += 1

    srs = DOCS / "srs" / "pulse-gen-firmware.md"
    if srs.exists():
        text = srs.read_text(encoding="utf-8")
        sections = re.split(r"^(## .+)$", text, flags=re.M)
        ids = []
        for i in range(1, len(sections), 2):
            title, body = sections[i], sections[i + 1]
            if re.match(r"## [2-6]\b", title):  # requirement definition chapters
                ids += re.findall(r"\|\s*([FITEN]-\d{3})\s*\|", body)
        dups = sorted({i for i in ids if ids.count(i) > 1})
        print(f"srs: {len(ids)} requirement rows, {len(set(ids))} distinct, dups={dups}")
        if dups:
            problems += 1

    # relative markdown links must resolve
    for doc in sorted(list(DOCS.rglob("*.md")) + ([GLOSSARY] if GLOSSARY.exists() else [])):
        text = doc.read_text(encoding="utf-8")
        for target in re.findall(r"\]\(([^)\s]+)\)", text):
            if target.startswith(("http://", "https://", "#", "mailto:")):
                continue
            path = (doc.parent / target.split("#")[0]).resolve()
            if not path.exists():
                print(f"BROKEN LINK {doc.relative_to(ROOT)} -> {target}")
                problems += 1

    print("OK" if not problems else f"{problems} problem(s)")
    return 0 if not problems else 1


if __name__ == "__main__":
    sys.exit(main())
