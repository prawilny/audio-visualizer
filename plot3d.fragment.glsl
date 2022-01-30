varying vec3 model_coord;

#define LIMIT1 0.037
#define LIMIT2 0.050
#define LIMIT3 0.075
#define LIMIT4 0.100
#define LIMIT5 0.150

#define COLOR2 vec3(0, 0, 255)
#define COLOR3 vec3(0, 255, 0)
#define COLOR4 vec3(255, 255, 0)
#define COLOR5 vec3(255, 128, 0)
#define COLOR6 vec3(255, 0, 0)

void main(void) {
	float frac = (model_coord.y + 1.0) / 2;
	vec3 color;
	if (frac < LIMIT1) {
		discard;
	} else if (frac < LIMIT2) {
		color = COLOR2;
	} else if (frac < LIMIT3) {
		color = COLOR3;
	} else if (frac < LIMIT4) {
		color = COLOR4;
	} else if (frac < LIMIT5) {
		color = COLOR5;
	} else {
		color = COLOR6;
	}
	gl_FragColor = vec4(color, model_coord.z);
}
