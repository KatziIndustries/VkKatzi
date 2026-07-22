#version 450

layout(location = 0) in vec2 inPosition;
layout(location = 1) in vec2 inOffset;
layout(location = 2) in vec2 inScale;
layout(location = 3) in vec3 inColor;

layout(location = 0) out vec3 fragColor; 

void main() {
    vec2 worldPos = inPosition * inScale + inOffset;
    gl_Position = vec4(worldPos, 0.0, 1.0);
    fragColor = inColor;
}