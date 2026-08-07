#version 450

layout(location = 0) in vec2 fragUV;

layout(push_constant) uniform PushConstants {
    mat4 ortho;
    vec2 mousePosition;
} pc;

layout(location = 0) out vec4 outColor;

layout(binding = 32) uniform sampler2D texSampler;

void main() {
    outColor = texture(texSampler, fragUV);
}