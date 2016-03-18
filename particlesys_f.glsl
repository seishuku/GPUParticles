#version 330

in vec2 UV;

uniform sampler2D Pos;
uniform sampler2D Velo;

uniform float TimeStep;
uniform vec2 Coords;
uniform float RandSeed;

layout(location=0) out vec4 oPos;
layout(location=1) out vec4 oVelo;

const vec3 Gravity=vec3(0.0, -9.81, 0.0);

const float TwoPi=6.28318530718;
uniform float SeedRadius=20.0;

uint hash(uint x)
{
	x+=(x<<10u);
	x^=(x>>6u);
	x+=(x<<3u);
	x^=(x>>11u);
	x+=(x<<15u);

	return x;
}

uint hash(uvec3 v)
{
	return hash(v.x^hash(v.y)^hash(v.z));
}

float random(vec3 v)
{
    return uintBitsToFloat(hash(floatBitsToUint(v))&0x007FFFFFu|0x3F800000u)-1.0;
}

void main()
{
	vec4 Position=texture(Pos, UV);
	vec4 Velocity=texture(Velo, UV);

	oPos.w=Position.w-TimeStep*0.25;

	if(Position.w<0.0)
	{
        float theta=random(vec3(UV.x, UV.y, RandSeed+1))*TwoPi;
        float r=random(vec3(UV.x, UV.y, RandSeed+2))*SeedRadius;

		oPos=vec4(0.0, -100.0, 0.0, random(vec3(UV.x, UV.y, RandSeed))*0.999+0.001);
		oVelo=vec4(r*cos(theta), random(vec3(UV.x, UV.y, RandSeed+3))*100.0+1.0, r*sin(theta), 0.0);
	}
	else
	{
		oPos.xyz=Position.xyz+Velocity.xyz*TimeStep;
		oVelo=Velocity+vec4(Gravity, 0.0)*(TimeStep*0.5);
	}
}
