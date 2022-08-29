
dev:
	cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
	$(MAKE) -C build

linux:
	cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
	$(MAKE) -C build

prof: linux
	LD_PRELOAD=/usr/lib/x86_64-linux-gnu/libprofiler.so.0 CPUPROFILE=/tmp/factropy.prof build/factropy --new
	google-pprof --web build/factropy /tmp/factropy.prof

leak: linux
	rm -f ~/tmp/fac*; make -j24 -C build/ && LD_PRELOAD="/usr/lib/x86_64-linux-gnu/libtcmalloc.so.4.5.3" HEAPPROFILE=~/tmp/factropy.hprof build/factropy
#	google-pprof build/factropy --web --base=$(HOME)/tmp/factropy.hprof.0068.heap $(HOME)/tmp/factropy.hprof.0279.heap

CC=gcc
CPP=g++
OBJECTS=$(shell ls -1 src/*.cc | sed 's/cc$$/o/g')

DEPS=imgui/imgui.o src/sdeflinfl.o src/par_shapes.o
DEPS_CFLAGS=$(shell sdl2-config --cflags) $(shell pkg-config --cflags freetype2)
DEPS_LFLAGS=$(shell sdl2-config --libs) $(shell pkg-config --libs freetype2)

dev2-x11: CFLAGS=-Wall -std=c++17 -Og -g1 -gz -femit-struct-debug-reduced $(DEPS_CFLAGS) -rdynamic
dev2-x11: LFLAGS=-lGL $(DEPS_LFLAGS) -ldl -pthread
dev2-x11: glew/glew-x11.o $(DEPS) $(OBJECTS)
	$(CPP) $(CFLAGS) -o factropy glew/glew-x11.o $(DEPS) $(OBJECTS) $(LFLAGS)

linux2-x11: CFLAGS=-Wall -std=c++17 -O3 -flto=jobserver -DNDEBUG $(DEPS_CFLAGS)
linux2-x11: LFLAGS=-lGL $(DEPS_LFLAGS) -ldl -pthread
linux2-x11: glew/glew-x11.o $(DEPS) $(OBJECTS)
	$(CPP) $(CFLAGS) -o factropy glew/glew-x11.o $(DEPS) $(OBJECTS) $(LFLAGS)

linux2-wayland: CFLAGS=-Wall -std=c++17 -O3 -flto=jobserver -DNDEBUG $(DEPS_CFLAGS)
linux2-wayland: LFLAGS=-lOpenGL $(DEPS_LFLAGS) -ldl -pthread
linux2-wayland: glew/glew-wayland.o $(DEPS) $(OBJECTS)
	$(CPP) $(CFLAGS) -o factropy glew/glew-wayland.o $(DEPS) $(OBJECTS) $(LFLAGS)

linux2PGOa-x11: CFLAGS=-Wall -std=c++17 -O3 -march=native -flto=jobserver -DNDEBUG $(DEPS_CFLAGS) -fprofile-generate
linux2PGOa-x11: LFLAGS=-lGL $(DEPS_LFLAGS) -ldl -pthread
linux2PGOa-x11: $(DEPS) $(OBJECTS)
	$(CPP) $(CFLAGS) -o factropy glew/glew-x11.o $(DEPS) $(OBJECTS) $(LFLAGS)

linux2PGOb-x11: CFLAGS=-Wall -std=c++17 -O3 -march=native -flto=jobserver -DNDEBUG $(DEPS_CFLAGS) -fprofile-use -fprofile-correction
linux2PGOb-x11: LFLAGS=-lGL $(DEPS_LFLAGS) -ldl -pthread -lgcov
linux2PGOb-x11: $(DEPS) $(OBJECTS)
	$(CPP) $(CFLAGS) -o factropy glew/glew-x11.o $(DEPS) $(OBJECTS) $(LFLAGS)

main.o: src/main.cc src/*.h
	$(CPP) $(CFLAGS) -c $< -o $@

src/%.o: src/%.cc src/*.h
	$(CPP) $(CFLAGS) -c $< -o $@

imgui/imgui.o: CFLAGS=-O3 -std=c++11 -Wall -Wno-subobject-linkage -DNDEBUG -Iimgui $(DEPS_CFLAGS)
imgui/imgui.o: imgui/build.cpp
	$(CPP) $(CFLAGS) -c $< -o $@

src/sdeflinfl.o: CFLAGS=-O3 -std=c11 -DNDEBUG
src/sdeflinfl.o: src/sdeflinfl.c
	$(CC) $(CFLAGS) -c $< -o $@

src/par_shapes.o: CFLAGS=-O3 -std=c11 -DNDEBUG
src/par_shapes.o: src/par_shapes.c
	$(CC) $(CFLAGS) -c $< -o $@

glew/glew-x11.o: CFLAGS=-O3 -std=c11 -DNDEBUG
glew/glew-x11.o: glew/glew.c
	$(CC) $(CFLAGS) -c $< -o glew/glew-x11.o

glew/glew-wayland.o: CFLAGS=-O3 -std=c11 -DNDEBUG -DGLEW_EGL
glew/glew-wayland.o: glew/glew.c
	$(CC) $(CFLAGS) -c $< -o glew/glew-wayland.o

clean:
	rm -f factropy factropy-test
	rm -rf build
	rm -f src/*.o test/*.o
	rm -f imgui/imgui.o
	rm -f glew/glew-x11.o
	rm -f glew/glew-wayland.o
	rm -f src/*.gcda

cleanPGOa:
	rm -f factropy
	rm -f src/*.o

SHELL:=/bin/bash
GTEST=googletest/googletest
GOBJECTS=$(shell ls -1 test/{flate,cat}*.cc | sed 's/cc$$/o/g')

gtest: CFLAGS=-O0 -std=c++17 -g -Wall -Werror -I$(GTEST)/include
gtest: LFLAGS=-lm -lpthread -ldl -lrt
gtest: $(GTEST)/src/gtest-all.o $(GTEST)/src/gtest_main.o $(GOBJECTS) test/glue.c src/sdeflinfl.o
	$(CPP) -o factropy-test test/glue.c $(GOBJECTS) src/sdeflinfl.o $(GTEST)/src/gtest-all.o $(GTEST)/src/gtest_main.o $(LFLAGS)

test/%.o: test/%.cc src/*.h src/*.cc
	$(CPP) $(CFLAGS) -c $< -o $@

$(GTEST)/src/gtest-all.o:
	$(CPP) -std=c++11 -isystem $(GTEST)/include -I$(GTEST) -c $(GTEST)/src/gtest-all.cc -o $(GTEST)/src/gtest-all.o

$(GTEST)/src/gtest_main.o:
	$(CPP) -std=c++11 -isystem $(GTEST)/include -I$(GTEST) -c $(GTEST)/src/gtest_main.cc -o $(GTEST)/src/gtest_main.o

utils:
	$(CPP) -std=c++17 -o build/flatecat util/flatecat.cc
