* Compile

Please compile on MIPS CI20.


Mode X11+OpenGL 2.1 (GLX)

$ g++ main_gl2.cpp -o app_with_gl2 -I. -lGL -lX11


Mode X11+EGL+GLES2.0

$ g++ main_gl2.cpp -o app_with_gles -I. -lGLESv2 -lEGL -lX11



It may be necessary. (as root user)
# apt-get install x11-dev libglu1-mesa-dev
