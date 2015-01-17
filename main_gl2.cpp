#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <iostream>
#include <vector>

//#define USE_GLES2

#ifndef USE_GLES2
 #define GL_GLEXT_PROTOTYPES (1)
 #include <GL/gl.h>
 #include <GL/glx.h>
 #include <GL/glxext.h>
#endif
#ifdef USE_GLES2
 #include "EGL/egl.h"
 #include "GLES2/gl2.h"
#endif


#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"


void CreateResource();
void DestroyResource();
void Draw();
double getDeltaTime( timeval* t0, timeval* t1 );

static const double PI = 3.14159265;

struct Vertex {
	float x, y, z;
	float u, v;
};

struct ResourceSet {
  GLuint shader;
  GLuint vboVB, vboIB;
  GLint  locPVW;
  GLint  indexCount;
};

Vertex gTriangle[] = 
{
	{ -1.0f,-1.0f, 0.0f,  0.0f, 0.0f },
	{ +1.0f,-1.0f, 0.0f,  1.0f, 0.0f },
	{  0.0f, 1.0f, 0.0f,  0.0f, 1.0f },
};
uint16_t gIndices[] = { 0, 1, 2 };
ResourceSet gResource;

Display* display = NULL;
Window    window;

#ifdef USE_GLES2
EGLDisplay eglDisplay;
EGLSurface eglSurface;
EGLContext eglContext;

void initializeEGL();
void terminateEGL();
void swapBuffers();
#else
GLXContext	glxContext;
void initializeOGL();
void terminateOGL();
void swapBuffers();
#endif

#ifndef USE_GLES2
// X11 + OpenGL 2.1 (GLX)
void swapBuffers() {
	glXSwapBuffers( display, window );
}
#else
// X11 + EGL + GLES2.0
void initializeEGL() {
	eglDisplay = eglGetDisplay( (EGLNativeDisplayType) display );
	if( eglDisplay == EGL_NO_DISPLAY ) {
		fprintf( stderr, "eglGetDisplay failed.\n" );
		return;
	}
	if( !eglInitialize( eglDisplay, NULL, NULL ) ) {
		fprintf( stderr, "eglInitialize failed.\n" );
		return;
	}
	
	EGLint attribList[] = {
		EGL_RED_SIZE, 8,
		EGL_GREEN_SIZE, 8,
		EGL_BLUE_SIZE, 8,
		EGL_ALPHA_SIZE, 8,
		EGL_DEPTH_SIZE, 24,
		EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
		EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
		EGL_NONE,
	};
	EGLint numConfigs;
	EGLConfig config;
	if( !eglGetConfigs( eglDisplay, NULL, 0, &numConfigs ) ) {
		fprintf( stderr, "eglGetConfigs failed.\n" );
	}
	eglChooseConfig( eglDisplay, attribList, &config, 1, &numConfigs );
	eglSurface = eglCreateWindowSurface( eglDisplay, config, (EGLNativeWindowType)window, NULL );
	
	EGLint contextAttrib[] = { EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE, EGL_NONE };
	eglContext = eglCreateContext( eglDisplay, config, EGL_NO_CONTEXT, contextAttrib );
	if( eglContext == EGL_NO_CONTEXT ) {
		fprintf( stderr, "eglCreateContext failed.\n" );
		return;
	}
	eglMakeCurrent( eglDisplay, eglSurface, eglSurface, eglContext );
}
void terminateEGL() {
	eglDestroyContext( eglDisplay, eglContext );
	eglDestroySurface( eglDisplay, eglSurface );
	eglTerminate( eglDisplay );
}
void swapBuffers() {
	eglSwapBuffers( eglDisplay, eglSurface );
}
#endif

int main( int argc, char* argv[] ) {
	int screen = 0;
	Window   root;
	Colormap cmap;
	XSetWindowAttributes swa;
	XWindowAttributes xwa;
	XEvent    xevt;
	int frame = 0;
	
	display = XOpenDisplay( NULL );
	if( display == NULL ) {
		fprintf( stderr, "cannot connect to X server\n" );
		return -1;
	}
	
	root = DefaultRootWindow( display );
#ifndef USE_GLES2
	GLint attr[] = {
		GLX_RGBA, GLX_DEPTH_SIZE, 24,
		GLX_DOUBLEBUFFER, None,
	};
	XVisualInfo* vinfo = glXChooseVisual( display, 0, attr );
	if( vinfo == NULL ) {
		fprintf( stderr, "no visual\n" );
		return -1;
	}
	
	cmap = XCreateColormap( display, root, vinfo->visual, AllocNone );
	swa.colormap = cmap;
	swa.event_mask = ExposureMask | KeyPressMask;
	window = XCreateWindow( display, root, 0, 0, 800, 600, 0, vinfo->depth, InputOutput, vinfo->visual, CWColormap | CWEventMask, &swa );
	
	XMapWindow( display, window );
	XStoreName( display, window, "SIMPLE APPLICATION" );
	
	glxContext = glXCreateContext( display, vinfo, NULL, GL_TRUE );
	glXMakeCurrent( display, window, glxContext );
#else
	swa.event_mask = ExposureMask | PointerMotionMask | KeyPressMask;
	window = XCreateWindow( display, root, 0, 0, 800, 600, 0, CopyFromParent, InputOutput, CopyFromParent, CWEventMask, &swa );

	Atom atom1, atom2;
	atom1 = XInternAtom( display, "WM_DELETE_WINDOW", False );
	atom2 = XInternAtom( display, "WM_PROTOCOLS", False );
	XSetWMProtocols( display, window, &(atom1), 1 );
	XMapWindow( display, window );
	XFlush( display );
	initializeEGL();

#endif

	GLint rb, gb, bb,ab, db;
	glGetIntegerv( GL_RED_BITS, &rb );
	glGetIntegerv( GL_GREEN_BITS, &gb );
	glGetIntegerv( GL_BLUE_BITS, &bb );
	glGetIntegerv( GL_ALPHA_BITS, &ab );
	glGetIntegerv( GL_DEPTH_BITS, &db );
	fprintf( stderr, "R:%d, G:%d, B:%d, A:%d, Depth:%d\n", rb, gb, bb, ab, db );
	fprintf( stderr, "GL_VERSION = %s\n", (const char*)glGetString( GL_VERSION ) );
	fprintf( stderr, "GL_RENDERER = %s\n", (const char*)glGetString( GL_RENDERER ) );

	CreateResource();

	bool isFinish = false;
	while( !isFinish ) {
		while( XPending( display ) ) {
			XNextEvent( display, &xevt );
			if( xevt.type == Expose ) {
				XGetWindowAttributes( display, window, &xwa );
				glViewport( 0, 0, xwa.width, xwa.height );
			} else if( xevt.type == KeyPress ) {
				XFlush( display );
				isFinish = true;
				break;
			}
		}
		if( isFinish ) {
			continue;
		}
		
		Draw();
		frame++;
		
		swapBuffers();
	}
	DestroyResource();
	
#ifdef USE_GLES2
	terminateEGL();
#else
	glXMakeCurrent( display, None, NULL );
	glXDestroyContext( display, glxContext );
#endif
	XDestroyWindow( display, window );
	XCloseDisplay( display );

	return 0;
}


void checkCompiled( GLuint shader ) {
  GLint status;
  glGetShaderiv( shader, GL_COMPILE_STATUS, &status );
  if( status != GL_TRUE ) {
    GLint length;
    glGetShaderiv( shader, GL_INFO_LOG_LENGTH, &length );
    if( length ) {
      char* buf = (char*)malloc( length );
      glGetShaderInfoLog( shader, length, NULL, buf );
      fprintf( stderr, "CompiledLog: %s\n", buf );
      free( buf );
    }
    exit( EXIT_FAILURE );
  }
  fprintf( stdout, "Compile Succeed.\n" );
}


const GLchar* srcVertexShader[] =
{
  "attribute vec4 position0;\n"
  "attribute vec2 texcoord0;\n"
  "varying vec4 vsout_color0;\n"
  "uniform vec4 matPVW[4];\n"
  "void main() {\n"
  "vec4 pos;\n"
  "  pos = matPVW[0] * position0.xxxx;\n"
  "  pos += matPVW[1]* position0.yyyy;\n"
  "  pos += matPVW[2]* position0.zzzz;\n"
  "  pos += matPVW[3]* position0.wwww;\n"
  "  gl_Position = pos;\n"
  "  vsout_color0 = vec4(texcoord0.xy, 0.0, 1.0);\n"
  "}"
 };
 
const GLchar* srcFragmentShader[] =
{
#ifdef USE_GLES2
  "precision mediump float;\n"
#endif
  "varying vec4 vsout_color0;\n"
  "void main() {\n"
  "  gl_FragColor = vsout_color0;\n"
  "}"
};
  
GLuint createShaderProgram() {
  GLuint shaderVS = glCreateShader( GL_VERTEX_SHADER );
  GLuint shaderFS = glCreateShader( GL_FRAGMENT_SHADER );

  
 
  glShaderSource( shaderVS, 1, srcVertexShader, 0 );
  int errCode = glGetError();
  if( errCode != GL_NO_ERROR ) {
	fprintf( stderr, "GLErr.  %X\n", errCode );
	exit(1);
  }

  glCompileShader( shaderVS );
  checkCompiled( shaderVS );
 
  glShaderSource( shaderFS, 1, srcFragmentShader, NULL );
  glCompileShader( shaderFS );
  checkCompiled( shaderFS );
 
  GLuint program;
  program = glCreateProgram();
  glAttachShader( program, shaderVS );
  glAttachShader( program, shaderFS );
 
  glLinkProgram( program );
 
  return program;
}

void CreateResource() {
	
	gResource.indexCount = sizeof(gIndices)/sizeof(gIndices[0]);
	gResource.shader = createShaderProgram();
	GLint locPos = glGetAttribLocation( gResource.shader, "position0" );
	GLint locUV = glGetAttribLocation( gResource.shader, "texcoord0" );
	gResource.locPVW = glGetUniformLocation( gResource.shader, "matPVW" );

	glGenBuffers( 1, &(gResource.vboVB) );
	glGenBuffers( 1, &(gResource.vboIB) );
	glBindBuffer( GL_ARRAY_BUFFER, gResource.vboVB );
	glBufferData( GL_ARRAY_BUFFER, sizeof(gTriangle), gTriangle, GL_STATIC_DRAW );

	glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, gResource.vboIB );
	glBufferData( GL_ELEMENT_ARRAY_BUFFER, sizeof(gIndices), gIndices, GL_STATIC_DRAW );
	
	char* offset = NULL;
	glVertexAttribPointer( locPos, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), offset );
	offset += sizeof(float)*3;
	glVertexAttribPointer( locUV, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), offset );
	offset += sizeof(float)*2;
	
	glEnableVertexAttribArray( locPos );
	glEnableVertexAttribArray( locUV );
}
void DestroyResource() {
	glDeleteBuffers( 1, &(gResource.vboVB) );
	glDeleteBuffers( 1, &(gResource.vboIB) );
	glDeleteProgram( gResource.shader );
}


double getTimeCount() {
	timeval t;
	gettimeofday( &t, NULL );
	return t.tv_sec + t.tv_usec * 1e-6;
}


void Draw() {
	static double prev = -1.0;
	double now = getTimeCount();
	if( prev < 0.0 ) {
		prev = now;
	}
	double dt = now - prev;
	
	static double angle = 0.0f;
    angle += 15.0 * dt;
	if( angle > 3600.0 ) {
		angle -= 3600.0;
	}
	glViewport( 0, 0, 640, 480 );
	
	glm::vec3 cameraPos = glm::vec3( 0.0, 0.0f, 10.0f );
	glm::mat4 proj = glm::perspective<float>( 30.0f, 640.0f/480.0f, 1.0f, 100.0f );
	glm::mat4 view = glm::lookAt<float>( cameraPos, glm::vec3(0.0f,0.0f,0.0f), glm::vec3(0.0f,1.0f,0.0f) );
	glm::mat4 world= glm::rotate( glm::mat4(1.0f), (float)angle, glm::vec3(0.0f,0.0f,1.0f) );
	world= glm::rotate( world, (float) angle * 0.5f, glm::vec3( 0.0f, 0.0f, 1.0f ) );

	glEnable( GL_DEPTH_TEST );
	glClearColor( 0.15f, 0.15f,0.25f, 0.0f );
	glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glUseProgram( gResource.shader );
    
    glBindBuffer( GL_ARRAY_BUFFER, gResource.vboVB );
    glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, gResource.vboIB );
    
    glm::mat4 pvw = proj * view * world;
    glUniform4fv( gResource.locPVW, 4, glm::value_ptr(pvw) );

    glDrawElements( GL_TRIANGLES, gResource.indexCount, GL_UNSIGNED_SHORT, NULL );
    
    prev = now;
}



