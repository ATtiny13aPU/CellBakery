#version 430 core

layout(location = 0) in vec4 position;
layout(location = 1) in vec4 color;
layout(location = 2) in vec4 meta;

out vec2 v_force;
out vec4 v_color;
out vec2 v_vel;

void main() {
    gl_Position = vec4(position.xy, 0.0, 1.0);
    v_vel = meta.xy;
	v_force = position.zw;
}