cmake_minimum_required(VERSION 3.12 FATAL_ERROR)

set(CMAKE_CXX_STANDARD 17)
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

project(PBR)

set(BUILD_SHARED_LIBS OFF CACHE INTERNAL "")

add_executable(${PROJECT_NAME})
target_include_directories(${PROJECT_NAME} PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/include
  # Neon math library.
  ${CMAKE_CURRENT_SOURCE_DIR}/external/Neon 
)
target_sources(${PROJECT_NAME} PUBLIC 
  ${CMAKE_CURRENT_SOURCE_DIR}/include/Debug.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/include/GlfwApp.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/include/Profiler.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/include/Geometry.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/include/ShaderProgram.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/include/Camera.hpp

  ${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/GlfwApp.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/Debug.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/Geometry.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/ShaderProgram.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/Camera.cpp
)

if (MSVC)
  set_property(
    TARGET ${PROJECT_NAME}
    PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/Resources"
  )
endif()

## OpenGL.
find_package(OpenGL REQUIRED)
target_link_libraries(${PROJECT_NAME} OpenGL::GL)

## glfw.
set(GLFW_BUILD_DOCS OFF CACHE INTERNAL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE INTERNAL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE INTERNAL "" FORCE)

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/external/glfw)
target_link_libraries(${PROJECT_NAME} glfw)

## gl3w
## Run glew_gen.py shipped by gl3w to generate the header and source files.
set(GL3W_HEADER_ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/external/gl3w/include)
if (NOT GL3W_FILES_GENERATED)
  execute_process(
    COMMAND python ${CMAKE_CURRENT_SOURCE_DIR}/external/gl3w/gl3w_gen.py
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/external/gl3w
  )
  set(GL3W_FILES_GENERATED ON CACHE INTERNAL "")
endif()
target_include_directories(${PROJECT_NAME} PUBLIC ${GL3W_HEADER_ROOT_DIR})
source_group(
  gl3w
  FILES ${CMAKE_CURRENT_SOURCE_DIR}/external/gl3w/src/gl3w.c
)
target_sources(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/external/gl3w/src/gl3w.c)

## dear imgui.
find_package(IMGUI REQUIRED)
target_include_directories(${PROJECT_NAME} PUBLIC ${IMGUI_INCLUDE_DIR})
source_group(
  DearImGui
  FILES ${IMGUI_SOURCES} ${IMGUI_HEADERS}
)
target_sources(${PROJECT_NAME} PUBLIC ${IMGUI_SOURCES})

## tinyobjloader.
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/external/tinyobjloader)
target_link_libraries(${PROJECT_NAME} tinyobjloader)
