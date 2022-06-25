#version 450

layout(binding = 1) uniform uniformBufferObject0
{
	vec3 pos;
} ubo0;

layout(location = 0) in vec2 in_position;
layout(location = 1) in vec2 in_uv;

layout(location = 1) out vec2 out_uv;

void main() {
    // gl_Position = vec4(positions[gl_VertexIndex], 0.0, 1.0);
    gl_Position = vec4(in_position + ubo0.pos.xy, 0.0, 1.0);
    out_uv = in_uv;
}