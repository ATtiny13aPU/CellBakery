#version 430 core

in vec2 dp;
in vec3 g_color;

out vec4 pixel;

void main() {
    float qd = dot(dp, dp);
    if (qd < 1.) {
        pixel = mix(vec4(g_color * 0.5, 0.8), vec4(g_color, 0.5), step(0.2 * 0.2, qd) * step(qd, 0.9 * 0.9));
		return;
    }
    discard;
}