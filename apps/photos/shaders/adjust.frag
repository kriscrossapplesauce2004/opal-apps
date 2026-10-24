#version 440
// Photo adjustments for the editor preview. Must match PhotoEditor::adjustPixels line for line.
layout(location = 0) in vec2 qt_TexCoord0;
layout(location = 0) out vec4 fragColor;
layout(std140, binding = 0) uniform buf {
    mat4 qt_Matrix;
    float qt_Opacity;
    float brightness;
    float contrast;
    float saturation;
    float warmth;
    float tint;
    float highlights;
    float shadows;
    float vignette;
    float fade;
};
layout(binding = 1) uniform sampler2D source;

const vec3 lumaW = vec3(0.2126, 0.7152, 0.0722);

void main()
{
    vec4 px = texture(source, qt_TexCoord0);
    vec3 c = px.a > 0.0 ? px.rgb / px.a : vec3(0.0);
    c = brightness >= 0.0 ? c + (1.0 - c) * brightness * 0.6 : c * (1.0 + brightness * 0.6);
    float lum = dot(c, lumaW);
    c += vec3(shadows * 0.35 * (1.0 - smoothstep(0.0, 0.5, lum)) + highlights * 0.35 * smoothstep(0.5, 1.0, lum));
    c = (c - 0.5) * (1.0 + contrast) + 0.5;
    c.r += warmth * 0.08;
    c.b -= warmth * 0.08;
    c.g += tint * 0.06;
    lum = dot(c, lumaW);
    c = vec3(lum) + (c - vec3(lum)) * (1.0 + saturation);
    c = c * (1.0 - fade) + fade;
    float d = length(qt_TexCoord0 - vec2(0.5)) * 1.41421;
    float f = smoothstep(0.35, 1.05, d);
    c = vignette >= 0.0 ? c * (1.0 - vignette * f * 0.75) : c + (1.0 - c) * (-vignette * f * 0.6);
    c = clamp(c, 0.0, 1.0);
    fragColor = vec4(c * px.a, px.a) * qt_Opacity;
}
