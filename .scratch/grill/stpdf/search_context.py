# -*- coding: utf-8 -*-
"""Search extracted PDF text for keywords; print page + context windows."""
import re, sys, io

def main(txt_path, keywords_file):
    text = io.open(txt_path, encoding='utf-8', errors='replace').read()
    pages = re.split(r'===PAGE (\d+)===', text)
    # pages: ['', '1', body1, '2', body2, ...]
    entries = []
    for i in range(1, len(pages), 2):
        pno = pages[i]
        body = pages[i+1] if i + 1 < len(pages) else ''
        entries.append((int(pno), body))
    keywords = [l.strip() for l in io.open(keywords_file, encoding='utf-8') if l.strip() and not l.startswith('#')]
    out = io.open(txt_path + '.hits.txt', 'w', encoding='utf-8')
    for kw in keywords:
        rx = re.compile(re.escape(kw), re.IGNORECASE)
        count = 0
        out.write('\n################ KEYWORD: %s ################\n' % kw)
        for pno, body in entries:
            for m in rx.finditer(body):
                s = max(0, m.start() - 350)
                e = min(len(body), m.end() + 350)
                out.write('\n----- page %d -----\n' % pno)
                out.write(body[s:e].replace('\x00', ' '))
                out.write('\n')
                count += 1
                if count >= 12:
                    break
            if count >= 12:
                break
        out.write('\n[TOTAL shown: %d]\n' % count)
    out.close()
    print('done ->', txt_path + '.hits.txt')

if __name__ == '__main__':
    main(sys.argv[1], sys.argv[2])
