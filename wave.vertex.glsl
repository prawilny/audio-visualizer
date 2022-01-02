attribute vec2 coord2d;
varying vec4 f_color;

#define LIMIT 0.2
#define FALL_FAST 0.25

void main(void) {
	gl_Position = vec4(coord2d.x, coord2d.y, 0, 1);
    f_color = vec4(1.0, 1.0, 1.0, 1.0);
}
