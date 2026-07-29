#version 450

layout(location = 0) in vec4 fragColor;

layout(push_constant) uniform PushConstants {
    mat4 ortho;
    vec2 mousePosition;
} pc;

layout(location = 0) out vec4 outColor;

void main() {
    float dist = distance(gl_FragCoord.xy, pc.mousePosition) / 300;
    float inverseDist = 1 - dist;

    outColor = vec4(fragColor.zyx, 1.0);
}