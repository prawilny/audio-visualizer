attribute vec2 coord2d;
varying vec4 f_color;

void main(void) {
	gl_Position = vec4(coord2d.x, coord2d.y, 0, 1);
	f_color = vec4(coord2d.x / 2.0 + 0.5, coord2d.y / 2.0 + 0.5, 1, 1);
	gl_PointSize = 4.0;
}
