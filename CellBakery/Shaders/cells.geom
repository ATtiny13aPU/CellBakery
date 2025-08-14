#version 430 core
#include "lib.glsl"

layout(points) in;
layout(triangle_strip, max_vertices = 4) out;

uniform vec4 ViewWorld;
uniform vec4 ViewWindow;
uniform vec2 WinSize;
uniform float TimeLerp;

in vec3 v_color[];
in vec2 v_vel[];
in float v_radius[];

out vec2 dp;
out vec3 g_color;



void main() {
    vec2 pos = gl_in[0].gl_Position.xy;
    g_color = v_color[0];
    float r = v_radius[0];

	pos += v_vel[0] * TimeLerp / 20.;

    // Преобразование мировой позиции в экранную
	vec2 mst = ViewWorld.zw - ViewWorld.xy;
	vec2 win_uv = mix(ViewWindow.xy, ViewWindow.zw, pos) * 2. - 1.;
    vec2 win_r = 1. / mst;

	//if (between(pos, ViewWorld.xy - 2., ViewWorld.zw + 2.) == 0.)
	//	return;

    gl_Position.zw = vec2(0.0, 1.0);

    // Четырехугольник
    vec2 p1 = win_uv - win_r, p2 = win_uv + win_r;

    // 1: bottom-left
    dp = vec2(-1);
    gl_Position.xy = p1;
    EmitVertex();
    // 2: bottom-right
    dp = vec2(1, -1);
    gl_Position.xy = vec2(p2.x, p1.y);
    EmitVertex();
    // 3: top-left
    dp = vec2(-1, 1);
    gl_Position.xy = vec2(p1.x, p2.y);
    EmitVertex();
    // 4: top-right
    dp = vec2(1);
    gl_Position.xy = p2;
    EmitVertex();

    EndPrimitive();
}