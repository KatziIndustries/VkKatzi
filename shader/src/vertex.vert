#version 450

layout(location = 0) in vec2 inPosition;
layout(location = 1) in vec2 inUV;

layout(push_constant) uniform PushConstants {
    mat4 ortho;
    vec2 mousePosition;
} pc;

layout(binding = 1) uniform TimeData {
    float time;
} timeData;

layout(location = 0) out vec2 fragUV; 

void main() {
    vec2 position = inPosition;

    position.x += timeData.time - 0.5;

    gl_Position = vec4(position, 0.0, 1.0);
    fragUV = inUV;
}