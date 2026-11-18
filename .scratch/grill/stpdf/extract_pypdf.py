# -*- coding: utf-8 -*-
"""Extract per-page text from ST PDFs using pypdf."""
import sys, io
from pypdf import PdfReader

def main(pdf_path, out_path):
    r = PdfReader(pdf_path)
    n = len(r.pages)
    out = io.open(out_path, 'w', encoding='utf-8', errors='replace')
    for i, page in enumerate(r.pages):
        out.write('===PAGE %d===\n' % (i + 1))
        try:
            t = page.extract_text() or ''
        except Exception as e:
            t = '[extract error: %s]' % e
        out.write(t)
        out.write('\n')
    out.close()
    print('pages:', n, '->', out_path)

if __name__ == '__main__':
    main(sys.argv[1], sys.argv[2])
