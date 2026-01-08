#version 430 core

layout(points) in;
layout(line_strip, max_vertices = 3) out;

uniform vec4 ViewWorld;
uniform vec4 ViewWindow;
uniform float ScaleForce;
uniform float ScaleVel;

in vec2 v_force[];
in vec2 v_vel[];


out float k;


void main() {
	vec2 pos = gl_in[0].gl_Position.xy;

	// Преобразование мировой позиции в экранную
	vec2 win_uv = mix(ViewWindow.xy, ViewWindow.zw, pos) * 2. - 1.;

	gl_Position.zw = vec2(0.0, 1.0);

	k = 0;
	gl_Position.xy = win_uv + v_force[0] * ScaleForce / (ViewWorld.zw - ViewWorld.xy);
	EmitVertex();

	k = 0.5;
	gl_Position.xy = win_uv;
	EmitVertex();
	
	k = 1.;
	gl_Position.xy = win_uv + v_vel[0] * ScaleVel / (ViewWorld.zw - ViewWorld.xy);
	EmitVertex();

	EndPrimitive();
}