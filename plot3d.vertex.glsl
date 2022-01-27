attribute vec2 coord2d;
varying vec4 graph_coord;
uniform mat4 vertex_transform;

void main(void) {
	graph_coord = vec4(coord2d, 0, 1);
	gl_Position = vertex_transform * graph_coord;
}
