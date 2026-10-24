import math, sys, pathlib
out = pathlib.Path(sys.argv[1])
def polar(lobes, depth, r=58, cx=64, cy=64, rot=0, n=360):
    pts=[]
    for i in range(n):
        a=i/n*2*math.pi
        rr=r*(1+depth*math.cos(lobes*(a-rot)))/(1+depth)
        pts.append((cx+math.cos(a)*rr, cy+math.sin(a)*rr))
    return "M"+" L".join(f"{x:.2f},{y:.2f}" for x,y in pts)+"Z"
def squircle(r=58, cx=64, cy=64, k=4.2, n=240):
    pts=[]
    for i in range(n):
        a=i/n*2*math.pi; c,s=math.cos(a),math.sin(a)
        x=cx+r*math.copysign(abs(c)**(2/k),c); y=cy+r*math.copysign(abs(s)**(2/k),s)
        pts.append((x,y))
    return "M"+" L".join(f"{x:.2f},{y:.2f}" for x,y in pts)+"Z"
def svg(name, shape, c1, c2, body):
    out.joinpath(name).write_text(f'''<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 128 128">
  <defs>
    <linearGradient id="bg" x1="0.15" y1="0" x2="0.85" y2="1"><stop offset="0" stop-color="{c1}"/><stop offset="1" stop-color="{c2}"/></linearGradient>
    <linearGradient id="sheen" x1="0" y1="0" x2="0" y2="1"><stop offset="0" stop-color="#ffffff" stop-opacity=".28"/><stop offset=".55" stop-color="#ffffff" stop-opacity="0"/></linearGradient>
  </defs>
  <path d="{shape}" fill="url(#bg)"/>
  <path d="{shape}" fill="url(#sheen)"/>
{body}
</svg>
''')
# Files: rounded square, a big folder with a darker back sheet.
svg("dev.opal.Files.svg", squircle(), "#ffd36b", "#f0a020",
 '''  <path d="M24 42a10 10 0 0 1 10-10h18.5a8 8 0 0 1 5.7 2.4L64 40h30a10 10 0 0 1 10 10v8H24z" fill="#8a5300"/>
  <rect x="24" y="50" width="80" height="50" rx="10" fill="#fff6dd"/>
  <rect x="24" y="50" width="80" height="12" rx="6" fill="#ffe7a6"/>''')
# Photos: eight-petal flower, a mountain range and a sun cut from a white card.
svg("dev.opal.Photos.svg", polar(8, .085, rot=math.pi/8), "#7fb2ff", "#3d67d6",
 '''  <rect x="28" y="32" width="72" height="64" rx="16" fill="#f3f7ff"/>
  <circle cx="81" cy="50" r="8.5" fill="#ffc94a"/>
  <path d="M28 86l20-24a5 5 0 0 1 7.6 0L70 80l7-8a5 5 0 0 1 7.5 0L100 89v-3 2a8 8 0 0 1 0 0v-2c0 5.5-4.5 10-10 10H44c-8.8 0-16-7.2-16-10z" fill="#2f57b8"/>''')
# Media: nine-lobed cookie, a fat rounded play triangle.
svg("dev.opal.Media.svg", polar(9, .07), "#ff8fbf", "#d93f7f",
 '''  <path d="M50 38.5c0-5 5.4-8.1 9.7-5.6l34 19.6c4.3 2.5 4.3 8.7 0 11.2l-34 19.6c-4.3 2.5-9.7-.6-9.7-5.6z" fill="#fff0f6" transform="translate(-6 7)"/>''')
# Terminal: pill-ish rounded square, a big prompt and cursor.
svg("dev.opal.Terminal.svg", squircle(k=3.2), "#3b4252", "#14171f",
 '''  <path d="M34 44l22 20-22 20" stroke="#7ff0b2" stroke-width="12" stroke-linecap="round" stroke-linejoin="round" fill="none"/>
  <rect x="62" y="78" width="34" height="12" rx="6" fill="#e9edf5"/>''')
# Text Editor: soft burst, a page with lines and a pencil across it.
svg("dev.opal.TextEditor.svg", polar(12, .045), "#7fe3c3", "#1f9c7c",
 '''  <rect x="32" y="26" width="58" height="76" rx="14" fill="#f2fffa"/>
  <rect x="43" y="42" width="36" height="7" rx="3.5" fill="#1f6e5b"/>
  <rect x="43" y="56" width="36" height="7" rx="3.5" fill="#1f6e5b"/>
  <rect x="43" y="70" width="20" height="7" rx="3.5" fill="#1f6e5b"/>
  <g transform="rotate(45 86 82)"><rect x="78" y="52" width="16" height="44" rx="5" fill="#ffc94a"/><path d="M78 96h16l-8 13z" fill="#ffe2a3"/><path d="M83 104h6l-3 5z" fill="#3a3a3a"/><rect x="78" y="52" width="16" height="9" rx="4" fill="#ff8a7a"/></g>''')
