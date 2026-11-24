#!/usr/bin/env python
# -*- coding: utf-8 -*-
"""Check documentation structure, placeholders, and relative links.

Checks
  1. no unresolved markers or leftover placeholders
  2. glossary term format and duplicate terms
  3. unique requirement IDs in the SRS
  4. relative links point to existing paths

Usage: python tools/doc_check.py
"""
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
DOCS = ROOT / "docs"

GLOSSARY = ROOT / "GLOSSARY.md"


def main() -> int:
    paths = sorted(list(DOCS.rglob("*.md")) + ([GLOSSARY] if GLOSSARY.exists() else []))
    documents = {path: path.read_text(encoding="utf-8") for path in paths}
    problems = 0
    for doc, text in documents.items():
        rel = doc.relative_to(ROOT)
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
        if bare:
            flags.append(f"{len(bare)} bare [待确认]")
        if leftovers:
            flags.append(f"leftover {leftovers}")
        if banned:
            flags.append("banned " + ", ".join(banned))
        if flags:
            problems += len(flags)
        print(
            f"  {str(rel):46s} CRC {crc:2d}"
            + ("   <-- " + "; ".join(flags) if flags else "")
        )

    if GLOSSARY in documents:
        g = documents[GLOSSARY]
        terms = re.findall(r"^\*\*(.+?)\*\*:", g, re.M)
        full = len(re.findall(r"^\*\*(.+?)\*\*\s*：", g, re.M))
        dups = {t for t in terms if terms.count(t) > 1}
        print(f"glossary: {len(terms)} terms, {full} full-width colons, dups={sorted(dups)}")
        if full or dups:
            problems += 1

    srs = DOCS / "srs" / "pulse-gen-firmware.md"
    if srs in documents:
        text = documents[srs]
        sections = re.split(r"^(## .+)$", text, flags=re.M)
        ids = []
        for i in range(1, len(sections), 2):
            title, body = sections[i], sections[i + 1]
            if re.match(r"## [2-6]\b", title):  # requirement definition chapters
                ids += re.findall(r"^\|\s*([FITEN]-\d{3})\s*\|", body, re.M)
        dups = sorted({i for i in ids if ids.count(i) > 1})
        print(f"srs: {len(ids)} requirement rows, {len(set(ids))} distinct, dups={dups}")
        if dups:
            problems += 1

    # relative markdown links must resolve
    for doc, text in documents.items():
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
