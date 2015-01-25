CC = mipsel-linux-gnu-gcc
CXX = mipsel-linux-gnu-g++
SYSROOT = ~/gcc48mipsci20-mingw-w64-i686/sys-root

RPATH += -Wl,-rpath $(SYSROOT)/lib/mipsel-linux-gnu
RPATH += -Wl,-rpath $(SYSROOT)/usr/lib/mipsel-linux-gnu


.PHONY: all clean

all: sampleApp_gl2 sampleApp_es2


sampleApp_gl2 : main_gl2.cpp
	$(CXX) -o $@ $< -I. -lX11 -lGL $(RPATH)

sampleApp_es2 : main_gl2.cpp
	$(CXX) -o $@ $< -DUSE_GLES2 -I. -lEGL -lGLESv2 -lX11 $(RPATH)

clean : 
	rm sampleApp_gl2 sampleApp_es2
