cmake_minimum_required(VERSION 3.16)

project(factropy)

if (MSYS)
	message(FATAL_ERROR "See Makefile.msys")
endif()

set(CMAKE_VERBOSE_MAKEFILE ON)
set(CMAKE_C_EXTENSIONS OFF)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_C_FLAGS "-Wall -Wno-unused-but-set-variable -fuse-ld=gold")
set(CMAKE_CXX_FLAGS "-Wall -Wno-subobject-linkage -fuse-ld=gold")
set(CMAKE_C_FLAGS_DEBUG "-Og -g -gz")
set(CMAKE_CXX_FLAGS_DEBUG "-Og -g -gz")
set(CMAKE_C_FLAGS_RELEASE "-O3 -DNDEBUG")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG")
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
set(CMAKE_C_COMPILE_OPTIONS_IPO -flto=jobserver)
set(CMAKE_CXX_COMPILE_OPTIONS_IPO -flto=jobserver)
set(CMAKE_ENABLE_EXPORTS TRUE)

file(GLOB SRC CONFIGURE_DEPENDS "src/*.cc")

find_package(SDL2 REQUIRED)
find_package(Freetype REQUIRED)
#amdgpu breaks find_package(OpenGL REQUIRED)

# sdeflinfl

set(CMAKE_C_STANDARD 11)
add_library(sdeflinfl OBJECT src/sdeflinfl.c)
target_include_directories(sdeflinfl PRIVATE)

# par_shapes

set(CMAKE_C_STANDARD 11)
add_library(par_shapes OBJECT src/par_shapes.c)
target_include_directories(par_shapes PRIVATE)

# imgui

set(CMAKE_CXX_STANDARD 11)
add_library(imgui OBJECT imgui/build.cpp)
target_include_directories(imgui PRIVATE ${SDL2_INCLUDE_DIRS} ${FREETYPE_INCLUDE_DIRS})

# glew

set(CMAKE_C_STANDARD 11)
add_library(glew OBJECT glew/glew.c)
target_include_directories(glew PRIVATE)

# factropy

set(CMAKE_CXX_STANDARD 20)
add_executable(factropy ${SRC})
target_include_directories(factropy PRIVATE ${SDL2_INCLUDE_DIRS})
target_link_libraries(factropy m pthread dl imgui sdeflinfl par_shapes ${SDL2_LIBRARIES} GL glew ${FREETYPE_LIBRARIES})
