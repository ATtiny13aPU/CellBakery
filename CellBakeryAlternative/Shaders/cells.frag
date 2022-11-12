#version 330 core

uniform float fTime;

in vec2 Pos;
in vec4 c_meta;
in vec4 c_pos;

out vec4 pixel;

uint ihash2(uvec2 s) {
	uint s1 = ((s.y ^ s.x) * 0xEC7269E5u + 0x4C8A248Du) ^ ((s.x >> 5u) * 0xC5EB9396u);
	s.x = ((s1 / 867u) * (s.x * 0x6C8FBCAFu)) ^ (s1 * (s.y & 0xF465C8F3u));
	
	return s.x;
}

const float t2kr[18] = float[](1., 1., 1., 1.3, 1., 1., 1., 1.3, 1.2, 1.15, 1.15, 1.15, 1., 1., 1., 1., 1., 1.1);

void main() {
	vec2 dp = Pos - c_pos.xy; // ������ �������
	float qd = dot(dp, dp); // ������� ���������� �� ������� ������
	float qr = c_pos.z * c_pos.z; // ������� ������� ������� ������
	int t = int(c_meta.a * 256.);


	if (qd < qr) {
		float r = length(dp / c_pos.z); // ������������� ������
		float a;
		int b = 0;

		// ��� ���������
		switch (t) {
			case(0): // �������
			case(1): // ��������
			case(3): // ���������  (fract(a * 19.) > 0.8);
				b = int(r < 0.20 || r > 0.90);
				break;
			case(2): // �������
				if (r < 0.20 || r > 0.90)
					b = 1;
				else {
					a = fract(atan(c_pos.y - Pos.y, c_pos.x - Pos.x) / 6.2831853 + c_pos.a);
					b = int(ihash2(uvec2(r * 5., a * 20.)) & 4u);
				}
				break;
			case(4): // �������
				b = (r < 0.20 || r > 0.93) ? 1 : int(r < 0.85) * 2;
				break;
			case(5): // �����������
				b = int((r < 0.2) || ((r > 0.7) && (r < 0.8)) || (r > 0.9));
				break;
			default:
				b = int(r > 0.9);
		}

		// ��� �������
		switch (b) {
			case(4): // ����������
				pixel = vec4(normalize(c_meta.rgb * vec3(0.5, 1.3, 0.3)) * 0.5, 0.90);
				break;
			case(3): // �������
				discard;
			case(2): // ��� ��������
				pixel = vec4((c_meta.rgb + vec3(0.5, 0.25, 0.25) * 3.) * 0.25, 0.90);
				break;
			case(1): // ����� �������
				pixel = vec4(c_meta.rgb * 0.5, 1.);
				break;
			default: // ������� �������
				pixel = vec4(c_meta.rgb, 0.67);
		}

	} else if (qd < qr * t2kr[t] * t2kr[t] || t == 1) {
		float r = length(dp / c_pos.z); // ������������� ������
		float a = fract(atan(c_pos.y - Pos.y, c_pos.x - Pos.x) / 6.2831853 + c_pos.a - 0.125);
		int b = 0;
		
		// ��� ���������
		switch (t) {
			case(1): // ��������
				b = int(abs(0.5 - a) - 0.02 < 0.);
				break;
			case(3): // ���������
				a = abs(fract(a * 19.) - 0.5);
				b = int((a - sqrt(r) * 1.2 > -0.865) || (1. / (0.85 - a) - (sqrt(r) - 1.) * 8. > 1.8));
				//b = int(mix(1. / (0.9 - a) - (r - 1.) * 3. + 1.7, a - sqrt(r) * 1.2 - 0.865, 0.5) > 0.79);
				//b = int((a - sqrt(r) * 1.2 > -0.865) || (a - r * 2. > -1.73));
				//b = int(1. / (0.9 - a) - (r - 1.) * 3. - 1.7 > 0. || a - sqrt(r) * 1.2 - 0.865 > 0.);
				break;
			case(8): // �������
				b = int(fract(a * 17.) - r * 0.1 > 0.7); //int((a + (0.1 / r) > 0.5)
				break;
		}

		// ��� �������
		switch (b) {
			case(2): // ����� �������
				pixel = vec4(c_meta.rgb * 0.5, 1.);
				break;
			case(1): // ����� �������
				pixel = vec4(c_meta.rgb * 0.5, 1.);
				break;
			default: // �������
				pixel = vec4(c_meta.rgb * 0.5, 0.2);
				//discard;
		}
	} else discard;
}


struct CellType {
int
	Phago,		// ������� 0
	Flagello,	// �������� 1
	Photo,		// ������� 2
	Devoro,		// ��������� 3
	Lipo,		// ������� 4
	Keratino,	// ����������� 5
	Buoyo,		// ������ 6
	Glueo,		// ��������� 7
	Viro,		// ������� 8
	Nitro,		// �������� 9
	Stereo,		// ��������� 10
	Senseo,		// ��������� 11
	Myo,		// ������ 12
	Neuro,		// �������� 13
	Secro,		// �������� 14
	Stemo,		// ��������� 15
	Gamete,		// ������ 16
	Cilio;		// �������� 17
};