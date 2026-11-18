# -*- coding: utf-8 -*-
"""Crude PDF text-layer extractor for ST PDFs (FlateDecode streams)."""
import re, zlib, sys

def parse_string(content, i):
    """Parse a (possibly nested/escaped) literal string starting at '(' at index i.
    Returns (bytes, next_index)."""
    depth = 1
    j = i + 1
    buf = bytearray()
    n = len(content)
    while j < n and depth > 0:
        ch = content[j:j+1]
        if ch == b'\\':
            nxt = content[j+1:j+2]
            if nxt in b'()\\':
                buf += nxt; j += 2; continue
            if nxt == b'n': buf += b'\n'; j += 2; continue
            if nxt == b'r': buf += b'\r'; j += 2; continue
            if nxt == b't': buf += b'\t'; j += 2; continue
            if nxt == b'b': buf += b'\b'; j += 2; continue
            if nxt == b'f': buf += b'\f'; j += 2; continue
            if nxt.isdigit():
                octv = content[j+1:j+4]
                try:
                    buf += bytes([int(octv, 8) & 0xFF])
                except ValueError:
                    buf += nxt
                j += 4; continue
            # unknown escape: keep next char
            buf += nxt; j += 2; continue
        if ch == b'(':
            depth += 1; buf += ch
        elif ch == b')':
            depth -= 1
            if depth > 0:
                buf += ch
        else:
            buf += ch
        j += 1
    return bytes(buf), j

def extract_text(content):
    out = bytearray()
    i = 0
    n = len(content)
    last_was_num = False
    while i < n:
        c = content[i:i+1]
        if c == b'(':
            s, i2 = parse_string(content, i)
            out += s
            i = i2
        elif c == b'<':
            j = content.find(b'>', i)
            if j != -1 and j - i > 2 and all(ch in b'0123456789abcdefABCDEF \r\n\t' for ch in content[i+1:j]):
                hexs = re.sub(rb'\s', b'', content[i+1:j])
                if len(hexs) % 2 == 0:
                    try:
                        out += bytes.fromhex(hexs.decode('ascii'))
                        i = j + 1
                        continue
                    except ValueError:
                        pass
            i += 1
        elif c == b'T':
            # lookahead: Tj / TJ - handled by string capture above; but add spacing between TJ elements
            pass
            i += 1
        else:
            i += 1
    return out.decode('latin-1', errors='replace')

def main(pdf_path, out_path):
    data = open(pdf_path, 'rb').read()

    # object table
    objs = {}
    offsets = {}
    for m in re.finditer(rb'(?m)(\d+)\s+(\d+)\s+obj\b', data):
        num = int(m.group(1))
        start = m.end()
        end = data.find(b'endobj', start)
        if end == -1:
            end = len(data)
        objs[num] = data[start:end]
        offsets[num] = m.start()

    def decompress(raw):
        raw = raw.strip(b'\r\n')
        if raw[:2] in (b'\x78\x9c', b'\x78\x01', b'\x78\xda', b'\x78\x5e'):
            try:
                return zlib.decompress(raw)
            except Exception:
                pass
        return None

    def text_of_stream(body):
        sm = re.search(rb'stream\r?\n(.*?)endstream', body, re.S)
        if not sm:
            return ''
        dec = decompress(sm.group(1))
        if dec is None:
            dec = sm.group(1)
        return extract_text(dec)

    def page_text(body):
        m = re.search(rb'/Contents\s*\[(.*?)\]', body, re.S)
        if m:
            refs = re.findall(rb'(\d+)\s+(\d+)\s+R', m.group(1))
            txt = ''
            for a, b in refs:
                if int(a) in objs:
                    txt += text_of_stream(objs[int(a)])
            if txt.strip():
                return txt
        m = re.search(rb'/Contents\s+(\d+)\s+(\d+)\s+R', body)
        if m:
            a = int(m.group(1))
            if a in objs:
                return text_of_stream(objs[a])
        # direct stream in page body
        sm = re.search(rb'stream\r?\n(.*?)endstream', body, re.S)
        if sm:
            dec = decompress(sm.group(1))
            if dec is None:
                dec = sm.group(1)
            return extract_text(dec)
        return ''

    page_nums = []
    for num, body in objs.items():
        if re.search(rb'/Type\s*/Page\b', body):
            page_nums.append(num)
    page_nums.sort(key=lambda n: offsets.get(n, -1))

    with open(out_path, 'w', encoding='utf-8', errors='replace') as f:
        for idx, num in enumerate(page_nums):
            txt = page_text(objs[num])
            f.write('===PAGE %d===\n' % (idx + 1))
            f.write(txt)
            f.write('\n')
    print('pages:', len(page_nums), '->', out_path)

if __name__ == '__main__':
    main(sys.argv[1], sys.argv[2])
