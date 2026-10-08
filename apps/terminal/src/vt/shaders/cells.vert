#version 440
layout(location = 0) in vec2 aPos;
layout(location = 1) in vec2 aUv;
layout(location = 2) in vec4 aColor;
layout(location = 0) out vec2 vUv;
layout(location = 1) out vec4 vColor;
layout(std140, binding = 0) uniform buf {
    mat4 qt_Matrix;
    float qt_Opacity;
};
void main()
{
    vUv = aUv;
    vColor = aColor * qt_Opacity;
    gl_Position = qt_Matrix * vec4(aPos, 0.0, 1.0);
}
