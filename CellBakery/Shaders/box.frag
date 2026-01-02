#version 430 core

out vec4 pixel;
in vec3 color;

void main() {
    pixel = vec4(color, 0.5);
}