#version 330

layout(location=0) in float Count;

uniform sampler2D Vertex;

uniform int ParticleSysSize;

uniform vec3 Color1=vec3(0.0, 0.125, 0.25);
uniform vec3 Color2=vec3(1.0, 0.25, 0.0);

out vec4 Color;

void main()
{
	// Use the current vertex count as an index into the square texture, x wraps around, y is scaled.
	// Could use gl_VertexID too, but might as well make use of the useless VBO.
	vec4 Pos=texture(Vertex, vec2(float(mod(Count, ParticleSysSize))/ParticleSysSize, float(Count/ParticleSysSize)/ParticleSysSize), 0.0);

	// Feed the texture data directly into the geometery shader.
	gl_Position=vec4(Pos.xyz, 1.0);

	// Color is just a simple mix using the particle "life", also passes life as alpha color (for fade).
	Color=vec4(mix(Color1, Color2, Pos.w), Pos.w);
}
