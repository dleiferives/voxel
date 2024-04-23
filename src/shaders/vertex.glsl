/////////////////////////////
precision mediump float;
layout(location = 0) in vec3 a_pos;
out vec3 f_color;
layout (std140) uniform ub_vp {
	mat4 projection;
	mat4 view;
};
float random(float seed) {
	return fract(sin(seed + a_pos.x + a_pos.y + a_pos.z) * 43758.5453123);
}
void main(){ // Note that the model position is the identity matrix for a mat4
	float index = random(float(gl_VertexID));
	gl_Position = projection * view * mat4(1.0) *  vec4(a_pos, 1.0);
	f_color = vec3(index,1.0 - index,index * 0.5);
}
