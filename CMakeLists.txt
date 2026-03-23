cmake_minimum_required(VERSION 3.15)

project(nanovg LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_C_STANDARD 11)

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
file(GLOB SRCS ${SRC_DIR}/*.cpp)
file(GLOB HDRS ${SRC_DIR}/*.h ${SRC_DIR}/*.hpp)

install(FILES ${HDRS} DESTINATION include/${PROJECT_NAME}/)

add_library(${PROJECT_NAME} ${SRCS})
target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_20)

set(DEFS "")
if(APPLE)
        find_library(cocoa_library Cocoa)
        find_library(opengl_library OpenGL)
        find_library(corevideo_library CoreVideo)
        find_library(iokit_library IOKit)
        set(LIBS  ${cocoa_library} ${opengl_library} ${corevideo_library} ${iokit_library})
elseif(WIN32)
        # Match premake: GLFW is "glfw3.lib"; GLEW static is "glew32s.lib" when GLEW_STATIC is defined.
        # Pass -DGLFW_ROOT=... and -DGLEW_ROOT=... to point at prebuilt SDKs (include/ + lib folders).
        set(WIN_SYS_LIBS gdi32 winmm user32 glu32 opengl32 kernel32)
        if(GLFW_ROOT AND GLEW_ROOT AND EXISTS "${GLFW_ROOT}/include/GLFW/glfw3.h" AND EXISTS "${GLEW_ROOT}/include/GL/glew.h")
                if(EXISTS "${GLFW_ROOT}/lib-vc2022/glfw3.lib")
                        set(GLFW_IMPORT_LIB "${GLFW_ROOT}/lib-vc2022/glfw3.lib")
                elseif(EXISTS "${GLFW_ROOT}/lib-vc2019/glfw3.lib")
                        set(GLFW_IMPORT_LIB "${GLFW_ROOT}/lib-vc2019/glfw3.lib")
                else()
                        message(FATAL_ERROR "GLFW_ROOT is set but no lib-vc2019/lib-vc2022/glfw3.lib found under ${GLFW_ROOT}")
                endif()
                set(GLEW_IMPORT_LIB "${GLEW_ROOT}/lib/Release/x64/glew32s.lib")
                if(NOT EXISTS "${GLEW_IMPORT_LIB}")
                        message(FATAL_ERROR "GLEW_ROOT is set but ${GLEW_IMPORT_LIB} not found (need x64 GLEW static lib)")
                endif()
                set(LIBS ${GLFW_IMPORT_LIB} ${GLEW_IMPORT_LIB} ${WIN_SYS_LIBS})
        else()
                set(LIBS glfw gdi32 winmm user32 GLEW glu32 opengl32 kernel32)
        endif()
        set(DEFS _CRT_SECURE_NO_WARNINGS NOMINMAX NDEBUG _CONSOLE GLEW_STATIC UNICODE _UNICODE NANOVG_GLEW)
else()
        set(LIBS glfw GLEW GLU GL Xext Xi Xrandr X11 Xxf86vm Xinerama Xcursor Xdamage dl z m)
endif()
set(COMMON_FILES "example/demo.cpp" "example/perf.cpp")

set(TARGETS ${PROJECT_NAME})
# OpenGL ES examples need GLES headers; typical Windows SDK + GLFW do not provide them.
if(WIN32)
	set(NANOVG_EXAMPLE_PROJECTS gl2 gl3 fbo)
else()
	set(NANOVG_EXAMPLE_PROJECTS gles2 gles3 gl2 gl3 fbo)
endif()
foreach(EXAMPLE_PROJ IN ITEMS ${NANOVG_EXAMPLE_PROJECTS})
        add_executable(example_${EXAMPLE_PROJ} "example/example_${EXAMPLE_PROJ}.cpp" ${COMMON_FILES})
        list(APPEND TARGETS example_${EXAMPLE_PROJ})
        target_compile_definitions(example_${EXAMPLE_PROJ} PUBLIC ${DEFS})
        target_link_libraries(example_${EXAMPLE_PROJ} ${PROJECT_NAME} ${LIBS})
        if(WIN32 AND GLFW_ROOT AND GLEW_ROOT AND EXISTS "${GLFW_ROOT}/include/GLFW/glfw3.h")
                target_include_directories(example_${EXAMPLE_PROJ} PUBLIC ${SRC_DIR} "${GLFW_ROOT}/include" "${GLEW_ROOT}/include")
        else()
                target_include_directories(example_${EXAMPLE_PROJ} PUBLIC ${SRC_DIR})
        endif()
        target_compile_features(example_${EXAMPLE_PROJ} PRIVATE cxx_std_20)
endforeach()

install(TARGETS ${TARGETS}
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        RUNTIME DESTINATION bin
        INCLUDES DESTINATION include/${PROJECT_NAME}
)

