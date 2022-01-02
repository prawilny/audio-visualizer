attribute vec2 coord2d;
varying vec4 f_color;

#define LIMIT 0.2
#define FALL_FAST 0.25

void main(void) {
	gl_Position = vec4(coord2d.x, coord2d.y, 0, 1);

    if (coord2d.y <= LIMIT) {
        float frac = coord2d.y / LIMIT;
        f_color = vec4(frac, 1.0, 0.5 * frac, 1.0);
    } else {
        float frac = (coord2d.y - LIMIT) / (1.0 - LIMIT);
        f_color = vec4(1.0, (1.0 - frac) * FALL_FAST, 0.5 * (1.0 - frac) * FALL_FAST, 1.0);
    }
}
