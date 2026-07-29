#version 450

layout(location = 0) in vec2 inPosition;
layout(location = 1) in vec4 inColor;

layout(binding = 0) uniform TimeData {
    float time;
} timeData;

layout(push_constant) uniform PushConstants {
    mat4 ortho;
    vec2 mousePosition;
} pc;

layout(location = 0) out vec4 fragColor; 

void main() {
    vec2 position = inPosition;
    position.x += timeData.time;

    vec2 mousePositionNDC = (pc.ortho * vec4(pc.mousePosition, 0.0, 1.0)).xy;

    //position += mousePositionNDC;

    gl_Position = vec4(position, 0.0, 1.0);
    fragColor = inColor;
}