varying vec3 model_coord;

void main(void) {
	float frac = model_coord.y;
	vec3 color = vec3(5 * frac, (1.0 - frac), 0);
	gl_FragColor = vec4(color, model_coord.z);
}
