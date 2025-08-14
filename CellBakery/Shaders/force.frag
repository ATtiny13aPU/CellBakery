#version 430 core

out vec4 pixel;
in float k;

void main() {
	const float k1 = abs(k - 0.5) + 0.5;
    pixel = mix(vec4(0.2, 1., 0.1, 1.), vec4(1.0, 0.2, 0.1, 1.), step(0.5, k)) * k1;
}