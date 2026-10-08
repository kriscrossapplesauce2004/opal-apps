#version 440
// Bloom: bright text gets a soft halo from a small blur of itself.
layout(location = 0) in vec2 qt_TexCoord0;
layout(location = 0) out vec4 fragColor;
layout(std140, binding = 0) uniform buf { mat4 qt_Matrix; float qt_Opacity; vec2 size; float time; vec4 glow; };
layout(binding = 1) uniform sampler2D source;
void main() {
    vec4 base = texture(source, qt_TexCoord0);
    vec2 px = 2.0 / max(size, vec2(1.0));
    vec3 halo = vec3(0.0);
    float w = 0.0;
    for (int x = -3; x <= 3; ++x)
        for (int y = -3; y <= 3; ++y) {
            vec3 s = texture(source, qt_TexCoord0 + vec2(x, y) * px).rgb;
            float k = exp(-float(x * x + y * y) / 6.0);
            halo += max(s - 0.45, 0.0) * k;
            w += k;
        }
    fragColor = vec4(base.rgb + halo / w * 2.2, base.a) * qt_Opacity;
}
