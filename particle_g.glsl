#version 330

layout(points) in;
layout(triangle_strip) out;
layout(max_vertices=4) out;

uniform mat4 MVP;
uniform vec3 Right;
uniform vec3 Up;

in vec4 Color[1];

out vec2 vUV;
out vec4 vColor;

void main()
{
	float Scale=10.0;					// Quad size.
	vec3 Pos=gl_in[0].gl_Position.xyz;	// Incoming vertex data.

	/* Quad as a single triangle strip:

		0 *----* 2
		  |   /|
		  |  / |
		  | /  |
		  |/   |
		1 *----* 3
	*/

	gl_Position=MVP*vec4(Pos-Right*Scale+Up*Scale, 1.0);
	vUV=vec2(0.0, 0.0);
	vColor=Color[0];
	EmitVertex();

	gl_Position=MVP*vec4(Pos-Right*Scale-Up*Scale, 1.0);
	vUV=vec2(0.0, 1.0);
	vColor=Color[0];
	EmitVertex();

	gl_Position=MVP*vec4(Pos+Right*Scale+Up*Scale, 1.0);
	vUV=vec2(1.0, 0.0);
	vColor=Color[0];
	EmitVertex();

	gl_Position=MVP*vec4(Pos+Right*Scale-Up*Scale, 1.0);
	vUV=vec2(1.0, 1.0);
	vColor=Color[0];
	EmitVertex();

	EndPrimitive();                                                                 
}
