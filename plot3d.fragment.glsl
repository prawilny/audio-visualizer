varying vec3 model_coord;

#define LIMIT0 0.000
#define LIMIT1 0.001
#define LIMIT2 0.050
#define LIMIT3 0.500
#define LIMIT4 1.000

#define COLOR0 vec3(0, 255, 0)
#define COLOR1 vec3(255, 255, 0)
#define COLOR2 vec3(255, 128, 0)
#define COLOR3 vec3(255, 0, 0)

void main(void) {
	float frac = model_coord.y;

	vec3 color;
	vec3 next_color;
	float next_frac;

	if (frac < LIMIT1) {
		color = COLOR0;
		next_color = COLOR1;
		next_frac = (frac - LIMIT0) / (LIMIT1 - LIMIT0);
	} else if (frac < LIMIT2) {
		color = COLOR1;
		next_color = COLOR2;
		next_frac = (frac - LIMIT1) / (LIMIT1 - LIMIT2);
	} else if (frac < LIMIT3) {
		color = COLOR2;
		next_color = COLOR3;
		next_frac = (frac - LIMIT2) / (LIMIT1 - LIMIT3);
	} else {
		color = COLOR3;
		next_color = COLOR3;
		next_frac = 0;
	}
	vec3 blended = color + (next_color - color) * next_frac;
	gl_FragColor = vec4(blended, model_coord.z);
}
