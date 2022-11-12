#version 330 core

out int id;

void main() {
	id = gl_InstanceID;
	gl_Position = vec4(0., 0., 0., 1.0);
}
