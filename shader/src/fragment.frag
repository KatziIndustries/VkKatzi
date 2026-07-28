#version 450

layout(location = 0) in vec4 fragColor;

layout(binding = 32) uniform MousePositon {
    vec2 mousePosition;
} MousePositionData;

layout(location = 0) out vec4 outColor;

void main() {
    float dist = distance(gl_FragCoord.xy, MousePositionData.mousePosition) / 200;
    float inverseDist = 2 - dist;

    outColor = vec4(fragColor.xyz, inverseDist);
}