cmake_minimum_required(VERSION 3.10)

set(PROJECT_NAME nanovg)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_C_STANDARD 11)

project(${PROJECT_NAME})

if (CMAKE_CXX_COMPILER_ID MATCHES "Clang" OR CMAKE_CXX_COMPILER_ID MATCHES "GNU")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DGL_GLEXT_PROTOTYPES=1 -O3 -Wall -Wreturn-type -Wno-reorder -Wno-implicit-fallthrough -Wno-unused-variable -Wno-unused-parameter -Wno-unused-function -Wno-deprecated -fmessage-length=0")
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DGL_GLEXT_PROTOTYPES=1 -O3 -Wall -Wno-unused-function -Wno-unused-variable -Wno-unused-parameter -Wno-deprecated")
endif()

if(WIN32)
  # Disable annoying MSVC warnings (all targets)
  add_definitions(/D "_CRT_SECURE_NO_WARNINGS")
  add_definitions(/D "NOMINMAX")
  add_definitions(/D "NDEBUG")
  add_definitions(/D "_CONSOLE")
  add_definitions(/D "GLEW_STATIC")
  add_definitions(/D "UNICODE")
  add_definitions(/D "_UNICODE")

  # Parallel build on MSVC (all targets)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP /Oi /Gy /Ob2 /W3 /openmp")

endif()

set(SRC_DIR "./src")
file(GLOB SRCS ${SRC_DIR}/*.c)
file(GLOB HDRS ${SRC_DIR}/*.h)

install(FILES ${HDRS} DESTINATION include/${PROJECT_NAME}/)

add_library(${PROJECT_NAME} ${SRCS})

set(DEFS "")
if(APPLE)
        find_library(cocoa_library Cocoa)
        find_library(opengl_library OpenGL)
        find_library(corevideo_library CoreVideo)
        find_library(iokit_library IOKit)
        set(LIBS  ${cocoa_library} ${opengl_library} ${corevideo_library} ${iokit_library})
elseif(WIN32)
        set(LIBS glfw gdi32 winmm user32 GLEW glu32 opengl32 kernel32)
        # NANOVG_GLEW 
        set(DEFS _CRT_SECURE_NO_WARNINGS NOMINMAX NDEBUG _CONSOLE GLEW_STATIC UNICODE _UNICODE)
else()
        set(LIBS glfw GLEW GLU GL Xext Xi Xrandr X11 Xxf86vm Xinerama Xcursor Xdamage dl z m)
endif()
set(COMMON_FILES "example/demo.c" "example/perf.c")

set(TARGETS ${PROJECT_NAME})
foreach(EXAMPLE_PROJ IN ITEMS gles2 gles3 gl2 gl3 fbo)
        add_executable(example_${EXAMPLE_PROJ} "example/example_${EXAMPLE_PROJ}.c" ${COMMON_FILES})
        list(APPEND ${TARGETS} example_${EXAMPLE_PROJ})
        target_compile_definitions(example_${EXAMPLE_PROJ} PUBLIC ${DEFS})
        target_link_libraries(example_${EXAMPLE_PROJ} ${PROJECT_NAME} ${LIBS})
        target_include_directories(example_${EXAMPLE_PROJ} PUBLIC ${SRC_DIR})
endforeach()

install(TARGETS ${TARGETS}
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        RUNTIME DESTINATION bin
        INCLUDES DESTINATION include/${PROJECT_NAME}
)

