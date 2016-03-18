#version 330

layout(location=0) in vec2 vPosition;

out vec2 UV;

void main()
{
	gl_Position=vec4(vPosition, -1.0, 1.0);
	UV=vec2(vPosition)*0.5+0.5;
}
