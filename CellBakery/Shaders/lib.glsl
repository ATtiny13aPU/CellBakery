

// Checks if `x` is in range [a, b].
float between(float x, float a, float b) {
	/// step(a, b) тоже самое, что и a <= b, 1 - step(a, b) тоже самое, что a > b
	/// тогда получается x >= a && b >= x, или же b >= x >= a что и есть [a, b]
	return step(0., step(a, x) * step(x, b));
}

float between(vec2 x, vec2 a, vec2 b) {
	const vec2 v = step(a, x) * step(x, b);
	return v.x * v.y;
}


float sqrLengthRect(vec2 tl, vec2 br, vec2 uv) {
	vec2 d = max(tl - uv, uv - br);
	d = max(vec2(0.), d) + min(0., max(d.x, d.y));
	return dot(d, d);
}