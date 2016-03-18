#version 330

in vec4 vColor;
in vec2 vUV;

uniform sampler2D Particle;

layout(location=0) out vec4 Output;

void main()
{
	float Alpha=texture(Particle, vUV).r;

	Output=vColor*vec4(1.0, 1.0, 1.0, Alpha);

	if(Alpha<0.001)
		discard;
}