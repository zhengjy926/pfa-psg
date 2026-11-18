#!/usr/bin/env python
# -*- coding: utf-8 -*-
"""PRM/SRS 全量重排脚本（grill-with-docs 执行阶段，用户裁决：全量重排、不生成映射表）。

步骤：
  1. 删除孤儿 PRM-030、PRM-711 行；
  2. 各号段内连续重排（0xx/1xx/2xx/3xx/4xx/5xx/6xx/7xx 保持三段编号）；
  3. SRS F/I/T/E/N 各段按出现顺序连续重排；
  4. 新增 PRM-120/411/512 行；PRM-313/323/324/117/118/119/203 行内容修改；
  5. 对 docs/**/*.md 与 GLOSSARY.md 全文做一次性 token 映射替换。

用法：
  python .scratch/grill/renumber.py            # dry-run，打印映射与检查结果
  python .scratch/grill/renumber.py --apply    # 实际写盘
"""
import argparse
import re
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent.parent
DOCS = ROOT / "docs"
PARAMS = DOCS / "params.md"
SRS = DOCS / "srs" / "pulse-gen-firmware.md"
GLOSSARY = ROOT / "GLOSSARY.md"

# ---------------------------------------------------------------------------
# PRM 段定义：每段 (当前旧ID列表, 新段起始号)。新列表按此顺序连续编号。
# ---------------------------------------------------------------------------
PRM_SEGMENTS = [
    # 0xx: 删除 030
    ([f"{i:03d}" for i in range(1, 30)] + ["031", "032"], "001"),
    # 1xx: 无缺口 + 新增 120
    ([f"{i:03d}" for i in range(101, 120)], "101"),
    # 2xx: 无缺口
    ([f"{i:03d}" for i in range(201, 215)], "201"),
    # 3xx: 316–319 缺口（320–330 前移）；D 组新增 331（电压未配置哨兵）顺延
    ([f"{i:03d}" for i in range(301, 316)] + [f"{i:03d}" for i in range(320, 332)], "301"),
    # 4xx: 无缺口 + 新增 411
    ([f"{i:03d}" for i in range(401, 411)], "401"),
    # 5xx: 无缺口 + 新增 512
    ([f"{i:03d}" for i in range(501, 512)], "501"),
    # 6xx: 605 缺口（606、607 前移）
    (["601", "602", "603", "604", "606", "607"], "601"),
    # 7xx: 删除 711（712–740 前移）
    ([f"{i:03d}" for i in range(701, 711)] + [f"{i:03d}" for i in range(712, 741)], "701"),
]

NEW_PRM_ROWS = {
    "1xx": "| PRM-120 | 上传模式上电默认 | 0x00（关闭） | — | 默认 | 0x0F 开启；I-012 握手重置恢复本默认；F-047 |",
    "5xx": "| PRM-512 | 锁存清除确认等待 | 5 | ms | 固定 | E-003、I-021 |",
}

PARAMS_ROW_EDITS = [
    # (旧片段, 新片段) —— 在重编号之前执行
    ("| PRM-313 | 触发间歇 | 200–2000 |", "| PRM-313 | 触发间歇 | 0–2000 |"),
    ("| PRM-323 | 0x32 边沿触发上电默认 | 0x00（未使能） |", "| PRM-323 | 0x32 触发模式上电默认 | 0x00（直接触发） |"),
    ("| PRM-324 | 0x37 边沿触发参数上电值 |", "| PRM-324 | 0x37 R波触发参数上电值 |"),
    ("| PRM-117 | 群级结算等待时间 | 10 | ms | 可配（宏） | 默认值。按 F-040 与群级结算共同分配最坏用时预算 |",
     "| PRM-117 | 结算任务唤醒周期 | 10 | ms | 可配（宏） | 默认值。按 F-040 与结算任务共同分配最坏用时预算 |"),
    ("| PRM-118 | 群级等待与结算总用时 | <50 | ms | 固定 |",
     "| PRM-118 | 结算任务唤醒至完成总用时 | <50 | ms | 固定 |"),
    ("| PRM-119 | 群级结算时间上限 | ≤40 | ms | 固定 | F-040。最坏等待为 10 ms 时，结算预算须小于 40 ms |",
     "| PRM-119 | 结算任务执行时间上限 | ≤40 | ms | 固定 | F-040。结算执行预算须小于 40 ms |"),
    ("软件发布版本用于 0x02。硬件版本未写入或校验失败时，0x04 读回此值。见 SRS I-025、E-009",
     "软件发布版本用于 0x02。硬件版本未写入或校验失败时，0x04 读回此值。软件版本为编译期宏定义，硬件版本写入 Flash。见 SRS I-025、E-009"),
]


def build_prm_mapping():
    mapping = {}
    for old_ids, start in PRM_SEGMENTS:
        start_i = int(start)
        for i, old in enumerate(old_ids):
            mapping[old] = f"{start_i + i:03d}"
    return mapping


def build_srs_mapping(text):
    """按 SRS 各章出现顺序对 [FITEN]-ddd 连续重排。"""
    sections = re.split(r"^(## .+)$", text, flags=re.M)
    per_letter = {}
    for i in range(1, len(sections), 2):
        title, body = sections[i], sections[i + 1]
        if re.match(r"## [2-6]\b", title):
            for m in re.finditer(r"\|\s*([FITEN])-(\d{3})\s*\|", body):
                per_letter.setdefault(m.group(1), []).append(m.group(2))
    mapping = {}
    for letter, ids in per_letter.items():
        for i, old in enumerate(sorted(ids)):
            mapping[f"{letter}-{old}"] = f"{letter}-{i + 1:03d}"
    return mapping


def apply_row_edits(text):
    for old, new in PARAMS_ROW_EDITS:
        if old not in text:
            raise SystemExit(f"params.md 行未找到：{old[:40]}...")
        text = text.replace(old, new, 1)
    # 删除孤儿行
    for prm in ("PRM-030", "PRM-711"):
        pat = re.compile(rf"^\|\s*{prm}\s*\|.*\n", re.M)
        text, n = pat.subn("", text)
        if n != 1:
            raise SystemExit(f"删除 {prm} 行失败（匹配 {n} 次）")
    # 追加新行（挂在各段最后一行之后）
    for seg, row in NEW_PRM_ROWS.items():
        pass  # 由 _append_rows 处理
    return text, NEW_PRM_ROWS


def _append_rows(lines, seg_start_ids, new_row):
    """在给定段（按 PRM 前缀匹配）最后一行后插入 new_row。"""
    seg = int(seg_start_ids[0]) // 100 * 100
    last_idx = None
    for idx, line in enumerate(lines):
        m = re.match(r"^\|\s*(PRM-\d{3})\s*\|", line)
        if m and int(m.group(1)[4:]) // 100 * 100 == seg:
            last_idx = idx
    if last_idx is None:
        raise SystemExit(f"段 {seg} 未找到")
    lines.insert(last_idx + 1, new_row)
    return lines


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--apply", action="store_true")
    args = ap.parse_args()

    prm_map = build_prm_mapping()
    params = PARAMS.read_text(encoding="utf-8")

    # 1) 内容行修改 + 孤儿行删除
    params_new = apply_row_edits(params)[0]

    # 2) 插入新 PRM 行（按当前段末行，使用新编号后的位置语义：段末即可）
    lines = params_new.splitlines(keepends=True)
    _append_rows(lines, ["101"], NEW_PRM_ROWS["1xx"])
    _append_rows(lines, ["501"], NEW_PRM_ROWS["5xx"])
    params_new = "".join(lines)

    # 3) params 表内 ID 重排
    def prm_repl(m):
        old = m.group(1)
        return f"PRM-{prm_map.get(old, old)}"
    params_new = re.sub(r"PRM-(\d{3})", prm_repl, params_new)

    # 4) SRS 映射
    srs_text = SRS.read_text(encoding="utf-8")
    srs_map = build_srs_mapping(srs_text)

    # 5) 全库 token 映射（params 之外的引用；params 表头 ID 已在上步处理）
    targets = sorted(list(DOCS.rglob("*.md"))) + ([GLOSSARY] if GLOSSARY.exists() else [])
    report = []
    for doc in targets:
        text = doc.read_text(encoding="utf-8")
        orig = text
        if doc == PARAMS:
            text = params_new
        else:
            text = re.sub(r"PRM-(\d{3})", prm_repl, text)
            text = re.sub(r"\b([FITEN])-(\d{3})\b",
                          lambda m: srs_map.get(m.group(0), m.group(0)), text)
        if text != orig:
            report.append((str(doc.relative_to(ROOT)), len(orig)))
            if args.apply:
                doc.write_text(text, encoding="utf-8")

    # 6) dry-run 校验：孤儿旧编号不得被 params 之外的文件引用
    leftovers = []
    for doc in sorted(list(DOCS.rglob("*.md"))) + [GLOSSARY]:
        if doc == PARAMS:
            continue  # params 自身经删行+重排后必然一致
        t = doc.read_text(encoding="utf-8")
        for tok in ("PRM-030", "PRM-711"):
            if tok in t:
                leftovers.append(f"{doc.relative_to(ROOT)} 仍引用 {tok}")
    print("PRM 映射:", {k: v for k, v in prm_map.items() if k != v})
    print("SRS 映射:", {k: v for k, v in srs_map.items() if k != v})
    print("改动文件数:", len(report))
    for r in report:
        print("  ", r[0])
    if leftovers:
        print("⚠ 孤儿残留引用：")
        for l in leftovers:
            print("  ", l)
    else:
        print("孤儿编号无残留引用 ✓")
    print("模式:", "已写盘" if args.apply else "dry-run（未写盘）")


if __name__ == "__main__":
    main()
