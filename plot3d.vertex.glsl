attribute vec3 coord3d;
varying vec3 model_coord;
uniform mat4 vertex_transform;

void main(void) {
	model_coord = coord3d;
	gl_Position = vertex_transform * vec4(model_coord, 1);
}
