#include <windows.h>
#include <process.h>
#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include "math.h"
#include "opengl.h"
#include "image.h"
#include "font.h"

#ifndef FREE
#define FREE(p) { if(p) { free(p); p=NULL; } }
#endif

#ifndef BUFFER_OFFSET
#define BUFFER_OFFSET(x) ((char *)NULL+(x))
#endif

GLContext_t Context;

char szAppName[]="OpenGL";

int Width=512, Height=512;

int Done=0, Key[256];

unsigned __int64 Frequency, StartTime, EndTime;
float avgfps=0.0f, fps=0.0f, fTimeStep, fTime=0.0f;
int Frames=0;

GLuint VAO;

typedef enum
{
	VBO_SCREENQUAD,
	VBO_PARTICLES,

	TEXTURE_PARTICLE,

	TEXTURE_PARTICLESYS_POS0,
	TEXTURE_PARTICLESYS_VELO0,
	TEXTURE_PARTICLESYS_POS1,
	TEXTURE_PARTICLESYS_VELO1,

	FBO_PARTICLESYS,

	GLSL_PARTICLESYS_SHADER,
	GLSL_PARTICLESYS_TIMESTEP,
	GLSL_PARTICLESYS_SEED,
	GLSL_PARTICLESYS_POS,
	GLSL_PARTICLESYS_VELO,

	GLSL_PARTICLE_SHADER,
	GLSL_PARTICLE_MVP,
	GLSL_PARTICLE_RIGHT,
	GLSL_PARTICLE_UP,
	GLSL_PARTICLE_TEXTURE,
	GLSL_PARTICLE_VTEXTURE,
	GLSL_PARTICLE_PARTICLESYSSIZE,

	NUM_OBJECTS
} ObjectNames;

unsigned int Objects[NUM_OBJECTS];

float RotateX=0.0f, RotateY=0.0f, PanX=0.0f, PanY=0.0f, Zoom=-300.0f;

float ModelView[16], Projection[16], MVP[16];
float QuatX[4], QuatY[4], Quat[4];

// Squared size for particle system (eg. 32*32), could be split into width/height as well.
const unsigned int ParticleSysSize=32;

unsigned long seed=0;

LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
void Render(void);
BOOL Init(void);
GLuint CreateProgram(char *VertexFilename, char *GeometryFilename, char *FragmentFilename);

unsigned __int64 rdtsc(void)
{
	unsigned long l, h;

	__asm
	{
		xor eax, eax
		xor ebx, ebx
		xor ecx, ecx
		xor edx, edx
		cpuid
		rdtsc
		mov l, eax
		mov h, edx
	}

	return ((unsigned __int64)l|((unsigned __int64)h<<32));
}

unsigned __int64 GetFrequency(void)
{
	unsigned __int64 TimeStart, TimeStop, TimeFreq;
	unsigned __int64 StartTicks, StopTicks;
	volatile unsigned __int64 i;

	QueryPerformanceFrequency((LARGE_INTEGER *)&TimeFreq);

	QueryPerformanceCounter((LARGE_INTEGER *)&TimeStart);
	StartTicks=rdtsc();

	for(i=0;i<1000000;i++);

	StopTicks=rdtsc();
	QueryPerformanceCounter((LARGE_INTEGER *)&TimeStop);

	return (StopTicks-StartTicks)*TimeFreq/(TimeStop-TimeStart);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int iCmdShow)
{
	WNDCLASS wc;
	MSG msg;RECT Rect;

	seed=_getpid();

	wc.style=CS_VREDRAW|CS_HREDRAW|CS_OWNDC;
	wc.lpfnWndProc=WndProc;
	wc.cbClsExtra=0;
	wc.cbWndExtra=0;
	wc.hInstance=hInstance;
	wc.hIcon=LoadIcon(NULL, IDI_WINLOGO);
	wc.hCursor=LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground=GetStockObject(BLACK_BRUSH);
	wc.lpszMenuName=NULL;
	wc.lpszClassName=szAppName;

	RegisterClass(&wc);

	SetRect(&Rect, 0, 0, Width, Height);
	AdjustWindowRect(&Rect, WS_OVERLAPPEDWINDOW, FALSE);

	Context.hWnd=CreateWindow(szAppName, szAppName, WS_OVERLAPPEDWINDOW|WS_CLIPSIBLINGS, CW_USEDEFAULT, CW_USEDEFAULT, Rect.right-Rect.left, Rect.bottom-Rect.top, NULL, NULL, hInstance, NULL);

	ShowWindow(Context.hWnd, SW_SHOW);
	SetForegroundWindow(Context.hWnd);

	if(!CreateContext(&Context, 32, 24, 0, 0, OGL_CORE33|OGL_DOUBLEBUFFER))
	{
		DestroyWindow(Context.hWnd);

		return -1;
	}

	if(!Init())
	{
		DestroyContext(&Context);
		DestroyWindow(Context.hWnd);

		return -1;
	}

	Frequency=GetFrequency();

	while(!Done)
	{
		if(PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			if(msg.message==WM_QUIT)
				Done=1;
			else
			{
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
		}
		else
		{
			StartTime=rdtsc();
			Render();
			SwapBuffers(Context.hDC);
			EndTime=rdtsc();

			fTimeStep=(float)(EndTime-StartTime)/Frequency;
			fTime+=fTimeStep;
			avgfps+=1.0f/fTimeStep;

			if(Frames++>100)
			{
				fps=avgfps/Frames;
				avgfps=0.0f;
				Frames=0;
			}
		}
	}

	DestroyContext(&Context);
	DestroyWindow(Context.hWnd);

	return msg.wParam;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	static POINT old;
	POINT pos, delta;

	switch(uMsg)
	{
		case WM_CREATE:
			break;

		case WM_CLOSE:
			PostQuitMessage(0);
			break;

		case WM_DESTROY:
			break;

		case WM_SIZE:
			Width=LOWORD(lParam);
			Height=HIWORD(lParam);
			break;

		case WM_LBUTTONDOWN:
		case WM_MBUTTONDOWN:
		case WM_RBUTTONDOWN:
			SetCapture(hWnd);
			ShowCursor(FALSE);

			GetCursorPos(&pos);
			old.x=pos.x;
			old.y=pos.y;
			break;

		case WM_LBUTTONUP:
		case WM_MBUTTONUP:
		case WM_RBUTTONUP:
			ShowCursor(TRUE);
			ReleaseCapture();
			break;

		case WM_MOUSEMOVE:
			GetCursorPos(&pos);

			if(!wParam)
			{
				old.x=pos.x;
				old.y=pos.y;
				break;
			}

			delta.x=pos.x-old.x;
			delta.y=old.y-pos.y;

			if(!delta.x&&!delta.y)
				break;

			SetCursorPos(old.x, old.y);

			switch(wParam)
			{
				case MK_LBUTTON:
					RotateX+=delta.x;
					RotateY-=delta.y;
					break;

				case MK_MBUTTON:
					PanX+=delta.x;
					PanY+=delta.y;
					break;

				case MK_RBUTTON:
					Zoom+=delta.y;
					break;
			}
			break;

		case WM_KEYDOWN:
			Key[wParam]=1;

			switch(wParam)
			{
				case VK_ESCAPE:
					PostQuitMessage(0);
					break;

				default:
					break;
			}
			break;

		case WM_KEYUP:
			Key[wParam]=0;
			break;
	}

	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

void InitParticles(void)
{
	float *Array=NULL;
	unsigned int i;

	// This data isn't actually used for rendering (vertex count).
	glGenBuffers(1, &Objects[VBO_PARTICLES]);
	glBindBuffer(GL_ARRAY_BUFFER, Objects[VBO_PARTICLES]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(float)*ParticleSysSize*ParticleSysSize, NULL, GL_DYNAMIC_DRAW);
	Array=(float *)glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);

	if(Array==NULL)
		return;

	for(i=0;i<ParticleSysSize*ParticleSysSize;i++)
		*Array++=(float)i;

	glUnmapBuffer(GL_ARRAY_BUFFER);

	// Generate and allocate the 4 needed floating point texture buffers

	// Position
	glGenTextures(1, &Objects[TEXTURE_PARTICLESYS_POS0]);
	glBindTexture(GL_TEXTURE_2D, Objects[TEXTURE_PARTICLESYS_POS0]);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, ParticleSysSize, ParticleSysSize, 0, GL_RGBA, GL_FLOAT, NULL);

	glGenTextures(1, &Objects[TEXTURE_PARTICLESYS_POS1]);
	glBindTexture(GL_TEXTURE_2D, Objects[TEXTURE_PARTICLESYS_POS1]);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, ParticleSysSize, ParticleSysSize, 0, GL_RGBA, GL_FLOAT, NULL);

	// Velocity
	glGenTextures(1, &Objects[TEXTURE_PARTICLESYS_VELO0]);
	glBindTexture(GL_TEXTURE_2D, Objects[TEXTURE_PARTICLESYS_VELO0]);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, ParticleSysSize, ParticleSysSize, 0, GL_RGBA, GL_FLOAT, NULL);

	glGenTextures(1, &Objects[TEXTURE_PARTICLESYS_VELO1]);
	glBindTexture(GL_TEXTURE_2D, Objects[TEXTURE_PARTICLESYS_VELO1]);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, ParticleSysSize, ParticleSysSize, 0, GL_RGBA, GL_FLOAT, NULL);

	glGenFramebuffers(1, &Objects[FBO_PARTICLESYS]);
}

GLuint BuildScreenQuadVBO(void)
{
	GLuint obj;
	float verts[]=
	{
		-1.0f, -1.0f,
		1.0f, -1.0f,
		1.0f, 1.0f,
		-1.0f, -1.0f,
		1.0f, 1.0f,
		-1.0f, 1.0f
	};

	glGenBuffers(1, &obj);
	glBindBuffer(GL_ARRAY_BUFFER, obj);
	glBufferData(GL_ARRAY_BUFFER, sizeof(float)*2*3*2, verts, GL_STATIC_DRAW);

	return obj;
}

void DrawScreenQuad(void)
{
	glBindBuffer(GL_ARRAY_BUFFER, Objects[VBO_SCREENQUAD]);

	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(float)*2, BUFFER_OFFSET(0));
	glEnableVertexAttribArray(0);

	glDrawArrays(GL_TRIANGLES, 0, 2*3);

	glDisableVertexAttribArray(0);
}

unsigned int CurrentParticleBuffer=0;

void UpdateParticles(void)
{
	GLenum DrawBuffers[]={ GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };

	glBindFramebuffer(GL_FRAMEBUFFER, Objects[FBO_PARTICLESYS]);
	glViewport(0, 0, ParticleSysSize, ParticleSysSize);

	glUseProgram(Objects[GLSL_PARTICLESYS_SHADER]);		// Particle system fragment shader

	glUniform1f(Objects[GLSL_PARTICLESYS_TIMESTEP], fTimeStep);			// Time delta between frames
	glUniform1f(Objects[GLSL_PARTICLESYS_SEED], (float)(seed+=3));	// Random seed, with increment

	glUniform1i(Objects[GLSL_PARTICLESYS_POS], 0);		// Texture sampler uniform for input position texture
	glUniform1i(Objects[GLSL_PARTICLESYS_VELO], 1);		// Texture sampler uniform for input velocity texture

	// Can't use the same texture as input while writing to it...
	// So we have to ping-pong textures and write buffer.
	// This can get rather large, 4 textures @ 1024x1024 RGBA floating point, 16MB each. Yikes.

	glDrawBuffers(2, DrawBuffers);

	if(CurrentParticleBuffer)
	{
		// buffer 0 = Write
		// buffer 1 = Read
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, Objects[TEXTURE_PARTICLESYS_POS1], 0);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, Objects[TEXTURE_PARTICLESYS_VELO1], 0);

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, Objects[TEXTURE_PARTICLESYS_POS0]);

		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, Objects[TEXTURE_PARTICLESYS_VELO0]);
	}
	else
	{
		// buffer 1 = Write
		// buffer 0 = Read
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, Objects[TEXTURE_PARTICLESYS_POS0], 0);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, Objects[TEXTURE_PARTICLESYS_VELO0], 0);

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, Objects[TEXTURE_PARTICLESYS_POS1]);

		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, Objects[TEXTURE_PARTICLESYS_VELO1]);
	}

	// Draw the full screen quad, processsing the particle system.
	DrawScreenQuad();

	// Unbind the FBO, since we're done with it.
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	// "Swap" buffers.
	CurrentParticleBuffer^=1;
}

void Render(void)
{
	UpdateParticles();

	glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

	// Set viewport and calculate a projection matrix (perspective, with infinite z-far plane)
	glViewport(0, 0, Width, Height);
	MatrixIdentity(Projection);
	InfPerspective(90.0f, (float)Width/Height, 0.01f, Projection);
	// ***

	// Set up model view matrix (translate and rotation)
	MatrixIdentity(ModelView);
	MatrixTranslate(PanX, PanY, Zoom, ModelView);

	QuatAngle(RotateX, 0.0f, 1.0f, 0.0f, QuatX);
	QuatAngle(RotateY, 1.0f, 0.0f, 0.0f, QuatY);
	QuatMultiply(QuatX, QuatY, Quat);
	QuatMatrix(Quat, ModelView);

	MatrixMult(ModelView, Projection, MVP);
	// ***

	// Render particle system

	// Bind "dummy" data VBO for rendering.
	glBindBuffer(GL_ARRAY_BUFFER, Objects[VBO_PARTICLES]);
	glVertexAttribPointer(0, 1, GL_FLOAT, GL_FALSE, sizeof(float), BUFFER_OFFSET(0));
	glEnableVertexAttribArray(0);

	glUseProgram(Objects[GLSL_PARTICLE_SHADER]);

	// Matrix data for general modelview/projection, as well as billboarding data.
	glUniformMatrix4fv(Objects[GLSL_PARTICLE_MVP], 1, GL_FALSE, MVP);
	glUniform3f(Objects[GLSL_PARTICLE_RIGHT], ModelView[0], ModelView[4], ModelView[8]);
	glUniform3f(Objects[GLSL_PARTICLE_UP], ModelView[1], ModelView[5], ModelView[9]);

	glUniform1i(Objects[GLSL_PARTICLE_TEXTURE], 0);		// Texture sampler for the actual particle texture
	glUniform1i(Objects[GLSL_PARTICLE_VTEXTURE], 1);	// Texture sampler for the vertex texture fetch

	glUniform1i(Objects[GLSL_PARTICLE_PARTICLESYSSIZE], ParticleSysSize);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, Objects[TEXTURE_PARTICLE]);			// Particle texture

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, Objects[TEXTURE_PARTICLESYS_POS0]);	// Vertex data

	glDisable(GL_DEPTH_TEST);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE);
	glEnable(GL_BLEND);

	// Uses geometery shader to produce billboarded quads,
	// doesn't however actually use any vertex data, gets that
	// from the particle system position texture and uses a vertex texture fetch.
	// The VBO is actually just dummy data.

	// The geometery shader could be written to do this as well,
	// so drawing one single point renders the whole system.
	// Still needs a VBO though.
	glDrawArrays(GL_POINTS, 0, ParticleSysSize*ParticleSysSize);

	glDisableVertexAttribArray(0);
	glDisableVertexAttribArray(1);
	// ***

	// Render stats
	glDisable(GL_DEPTH_TEST);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_BLEND);
		Font_Print(0.0f, 0.0f, "FPS: %0.1f", fps);
	glDisable(GL_BLEND);
	glEnable(GL_DEPTH_TEST);
	// ***
}

BOOL Init(void)
{
	InitParticles();

	Objects[VBO_SCREENQUAD]=BuildScreenQuadVBO();

	Objects[TEXTURE_PARTICLE]=Image_Upload("particle.tga", IMAGE_MIPMAP|IMAGE_BILINEAR);

	// For rendering the particle system
	Objects[GLSL_PARTICLE_SHADER]=CreateProgram("particle_v.glsl", "particle_g.glsl", "particle_f.glsl");
	glUseProgram(Objects[GLSL_PARTICLE_SHADER]);
	Objects[GLSL_PARTICLE_MVP]=glGetUniformLocation(Objects[GLSL_PARTICLE_SHADER], "MVP");
	Objects[GLSL_PARTICLE_RIGHT]=glGetUniformLocation(Objects[GLSL_PARTICLE_SHADER], "Right");
	Objects[GLSL_PARTICLE_UP]=glGetUniformLocation(Objects[GLSL_PARTICLE_SHADER], "Up");
	Objects[GLSL_PARTICLE_TEXTURE]=glGetUniformLocation(Objects[GLSL_PARTICLE_SHADER], "Particle");
	Objects[GLSL_PARTICLE_VTEXTURE]=glGetUniformLocation(Objects[GLSL_PARTICLE_SHADER], "Vertex");
	Objects[GLSL_PARTICLE_PARTICLESYSSIZE]=glGetUniformLocation(Objects[GLSL_PARTICLE_SHADER], "ParticleSysSize");

	Objects[GLSL_PARTICLESYS_SHADER]=CreateProgram("particlesys_v.glsl", NULL, "particlesys_f.glsl");
	glUseProgram(Objects[GLSL_PARTICLESYS_SHADER]);
	Objects[GLSL_PARTICLESYS_TIMESTEP]=glGetUniformLocation(Objects[GLSL_PARTICLESYS_SHADER], "TimeStep");
	Objects[GLSL_PARTICLESYS_SEED]=glGetUniformLocation(Objects[GLSL_PARTICLESYS_SHADER], "RandSeed");
	Objects[GLSL_PARTICLESYS_POS]=glGetUniformLocation(Objects[GLSL_PARTICLESYS_SHADER], "Pos");
	Objects[GLSL_PARTICLESYS_VELO]=glGetUniformLocation(Objects[GLSL_PARTICLESYS_SHADER], "Velo");

	// Set OpenGL states
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);

	// Dummy VAO, because I don't use them :P
	glGenVertexArrays(1, &VAO);
	glBindVertexArray(VAO);

	return TRUE;
}

int LoadShader(GLuint Shader, char *Filename)
{
	FILE *stream;
	char *buffer;
	int length;

	if((stream=fopen(Filename, "rb"))==NULL)
		return 0;

	fseek(stream, 0, SEEK_END);
	length=ftell(stream);
	fseek(stream, 0, SEEK_SET);

	buffer=(char *)malloc(length+1);
	fread(buffer, 1, length, stream);
	buffer[length]='\0';

	glShaderSource(Shader, 1, (const char **)&buffer, NULL);

	fclose(stream);
	free(buffer);

	return 1;
}

GLuint CreateProgram(char *VertexFilename, char *GeometryFilename, char *FragmentFilename)
{
	GLuint Program, Vertex, Geometry, Fragment;
	GLint Status, LogLength;
	char *Log=NULL;

	Program=glCreateProgram();

	if(VertexFilename)
	{
		Vertex=glCreateShader(GL_VERTEX_SHADER);

		if(LoadShader(Vertex, VertexFilename))
		{
			glCompileShader(Vertex);
			glGetShaderiv(Vertex, GL_COMPILE_STATUS, &Status);

			if(!Status)
			{
				glGetShaderiv(Vertex, GL_INFO_LOG_LENGTH, &LogLength);
				Log=(char *)malloc(LogLength);

				if(Log)
				{
					glGetShaderInfoLog(Vertex, LogLength, NULL, Log);
					MessageBox(Context.hWnd, Log, VertexFilename, MB_OK);
					free(Log);
				}
			}
			else
				glAttachShader(Program, Vertex);
		}

		glDeleteShader(Vertex);
	}

	if(GeometryFilename)
	{
		Geometry=glCreateShader(GL_GEOMETRY_SHADER);

		if(LoadShader(Geometry, GeometryFilename))
		{
			glCompileShader(Geometry);
			glGetShaderiv(Geometry, GL_COMPILE_STATUS, &Status);

			if(!Status)
			{
				glGetShaderiv(Geometry, GL_INFO_LOG_LENGTH, &LogLength);
				Log=(char *)malloc(LogLength);

				if(Log)
				{
					glGetShaderInfoLog(Geometry, LogLength, NULL, Log);
					MessageBox(Context.hWnd, Log, GeometryFilename, MB_OK);
					free(Log);
				}
			}
			else
				glAttachShader(Program, Geometry);
		}

		glDeleteShader(Geometry);
	}

	if(FragmentFilename)
	{
		Fragment=glCreateShader(GL_FRAGMENT_SHADER);

		if(LoadShader(Fragment, FragmentFilename))
		{
			glCompileShader(Fragment);
			glGetShaderiv(Fragment, GL_COMPILE_STATUS, &Status);

			if(!Status)
			{
				glGetShaderiv(Fragment, GL_INFO_LOG_LENGTH, &LogLength);
				Log=(char *)malloc(LogLength);

				if(Log)
				{
					glGetShaderInfoLog(Fragment, LogLength, NULL, Log);
					MessageBox(Context.hWnd, Log, FragmentFilename, MB_OK);
					free(Log);
				}
			}
			else
				glAttachShader(Program, Fragment);
		}

		glDeleteShader(Fragment);
	}

	glLinkProgram(Program);
	glGetProgramiv(Program, GL_LINK_STATUS, &Status);

	if(!Status)
	{
		glGetProgramiv(Program, GL_INFO_LOG_LENGTH, &LogLength);
		Log=(char *)malloc(LogLength);

		if(Log)
		{
			glGetProgramInfoLog(Program, LogLength, NULL, Log);
			MessageBox(Context.hWnd, Log, "Link", MB_OK);
			free(Log);
		}
	}

	return Program;
}
