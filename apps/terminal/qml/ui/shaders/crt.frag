#version 440
// CRT look: a little barrel curve, scanlines, a slight colour split and a vignette.
layout(location = 0) in vec2 qt_TexCoord0;
layout(location = 0) out vec4 fragColor;
layout(std140, binding = 0) uniform buf { mat4 qt_Matrix; float qt_Opacity; vec2 size; float time; vec4 glow; };
layout(binding = 1) uniform sampler2D source;
void main() {
    vec2 uv = qt_TexCoord0 * 2.0 - 1.0;
    uv *= 1.0 + 0.035 * dot(uv.yx, uv.yx);
    vec2 tc = uv * 0.5 + 0.5;
    if (tc.x < 0.0 || tc.x > 1.0 || tc.y < 0.0 || tc.y > 1.0) { fragColor = vec4(0.0); return; }
    float px = 1.0 / max(size.x, 1.0);
    vec3 c;
    c.r = texture(source, tc + vec2(px * 0.8, 0.0)).r;
    c.g = texture(source, tc).g;
    c.b = texture(source, tc - vec2(px * 0.8, 0.0)).b;
    float a = texture(source, tc).a;
    float scan = 0.82 + 0.18 * sin(tc.y * size.y * 3.14159);
    float vig = smoothstep(1.35, 0.55, length(uv));
    fragColor = vec4(c * scan * vig * 1.08, a) * qt_Opacity;
}
