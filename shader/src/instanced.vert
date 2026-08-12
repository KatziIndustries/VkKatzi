#version 450

layout(location = 0) in vec2 inPos;

layout(location = 1) in vec2 inInstanceOffset;
layout(location = 2) in vec3 inInstanceColor;

layout(location = 0) out vec3 fragColor;

void main() {
    vec2 worldPos = inPos + inInstanceOffset;
    gl_Position = vec4(worldPos, 0.0, 1.0);
    fragColor = inInstanceColor;
}