#version 450

layout(location = 0) in vec2 in_position;
layout(location = 1) in vec2 in_uv;

vec3 colors[4] = vec3[](
    vec3(1.0, 0.0, 0.0),
    vec3(0.0, 1.0, 0.0),
    vec3(0.0, 0.0, 1.0),
    vec3(0.0, 0.0, 0.0)
);

layout(location = 0) out vec3 out_color;
layout(location = 1) out vec2 out_uv;

void main() {
    // gl_Position = vec4(positions[gl_VertexIndex], 0.0, 1.0);
    gl_Position = vec4(in_position, 0.0, 1.0);
    out_color = colors[gl_VertexIndex];
    out_uv = in_uv;
}