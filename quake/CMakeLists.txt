# $Id: CMakeLists.txt 1320 2014-10-26 08:12:52Z justin $

set(LFL_PNG 1)
set(LFL_BOX2D 1)
set(LFL_LIBARCHIVE 1)

if(LFL_ANDROID)
 set(LFL_GLES2 1)
elseif(LFL_IPHONE)
 set(LFL_GLES2 1)
else(LFL_ANDROID)
 set(LFL_GLEW 1)
 set(LFL_GLFWINPUT 1)
 set(LFL_GLFWVIDEO 1)
 set(LFL_PORTAUDIO 1)
 if(NOT WIN32)
 set(LFL_GLOG 1)
 endif(NOT WIN32)
endif(LFL_ANDROID)

lfl_project(quake)
add_subdirectory(../lfapp ${CMAKE_CURRENT_BINARY_DIR}/lfapp_obj)
add_definitions(${LFAPP_DEF})
include_directories(${LFAPP_INCLUDE})

if(LFL_ANDROID)
add_library(quake quake.cpp)
else(LFL_ANDROID)
add_executable(quake quake.cpp ${SDL_MAIN} ${PROTO_SRC})
target_link_libraries(quake ${LFAPP_LIB})
endif(LFL_ANDROID)

