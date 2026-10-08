#!/usr/bin/env python3
"""Builds a labelled contact sheet from rendered prototypes: contact-sheet.py out.png title "label=path" ..."""
import sys
from PIL import Image, ImageDraw, ImageFont

def font(size, bold=False):
    for name in ("/usr/local/share/fonts/g/GoogleSansFlex_VariableFont_GRAD,ROND,opsz,slnt,wdth,wght.ttf", "/usr/share/fonts/TTF/DejaVuSans.ttf"):
        try:
            f = ImageFont.truetype(name, size)
            if bold:
                try: f.set_variation_by_axes([0, 100, size, 0, 100, 700])
                except Exception: pass
            return f
        except OSError:
            continue
    return ImageFont.load_default()

out, title, entries = sys.argv[1], sys.argv[2], [e.split("=", 1) for e in sys.argv[3:]]
row_h, gap, pad = 620, 36, 56
tiles = []
for label, path in entries:
    im = Image.open(path).convert("RGB")
    scale = row_h / im.height
    tiles.append((label, im.resize((round(im.width * scale), row_h), Image.LANCZOS)))
# Two rows: desktop-sized renders first, then the rest.
rows, current, width_limit = [], [], 3000
for t in tiles:
    if current and sum(x[1].width for x in current) + gap * len(current) + t[1].width > width_limit:
        rows.append(current); current = []
    current.append(t)
rows.append(current)
W = max(sum(x[1].width for x in r) + gap * (len(r) - 1) for r in rows) + pad * 2
H = pad + 90 + len(rows) * (row_h + 70 + gap) + pad
sheet = Image.new("RGB", (W, H), (14, 13, 11))
d = ImageDraw.Draw(sheet)
d.text((pad, pad), title, font=font(54, True), fill=(236, 232, 210))
y = pad + 100
for r in rows:
    x = pad
    for label, im in r:
        size = 30
        while size > 14 and d.textlength(label, font=font(size, True)) > im.width - 8:
            size -= 2
        d.text((x + 4, y + (30 - size)), label, font=font(size, True), fill=(205, 200, 160))
        sheet.paste(im, (x, y + 52))
        x += im.width + gap
    y += row_h + 70 + gap
sheet.save(out)
print(out)
