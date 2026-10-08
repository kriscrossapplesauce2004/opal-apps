#version 440
// Edge glow: an inner glow along the pane border in the accent colour.
layout(location = 0) in vec2 qt_TexCoord0;
layout(location = 0) out vec4 fragColor;
layout(std140, binding = 0) uniform buf { mat4 qt_Matrix; float qt_Opacity; vec2 size; float time; vec4 glow; };
layout(binding = 1) uniform sampler2D source;
void main() {
    vec4 base = texture(source, qt_TexCoord0);
    vec2 p = qt_TexCoord0 * size;
    float d = min(min(p.x, size.x - p.x), min(p.y, size.y - p.y));
    float g = exp(-d / 18.0) * (0.75 + 0.25 * sin(time * 1.4));
    fragColor = vec4(base.rgb + glow.rgb * g * 0.55, base.a) * qt_Opacity;
}
