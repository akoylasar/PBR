#.rst:
# FindIMGUI
# --------
#
# Find IMGUI
#
# Find the imgui includes and library.
#
# ::
#
#   IMGUI_INCLUDE_DIR, Where to find imgui headers.
#   IMGUI_HEADERS, List of headers files.
#   IMGUI_SOURCES, Lost of source files.
#   IMGUI_FOUND, If false, do not try to use imgui.
#
# ::
#

find_path(IMGUI_INCLUDE_DIR
  NAMES
  imconfig.h
  imgui.h
  imgui_internal.h
  imstb_rect_pack.h
  imstb_textedit.h
  imstb_truetype.h
  imgui_impl_opengl3.h
  imgui.cpp
  imgui_tables.cpp
  imgui_widgets.cpp
  imgui_demo.cpp
  imgui_draw.cpp
  imgui_impl_opengl3.cpp
  PATHS ${CMAKE_CURRENT_SOURCE_DIR}/external/imgui
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(IMGUI REQUIRED_VARS IMGUI_INCLUDE_DIR)

if(IMGUI_FOUND)
  message(STATUS "Found ImGui.")
  set(IMGUI_HEADERS
    ${IMGUI_INCLUDE_DIR}/imconfig.h
    ${IMGUI_INCLUDE_DIR}/imgui.h
    ${IMGUI_INCLUDE_DIR}/imgui_internal.h
    ${IMGUI_INCLUDE_DIR}/imstb_rect_pack.h
    ${IMGUI_INCLUDE_DIR}/imstb_textedit.h
    ${IMGUI_INCLUDE_DIR}/imstb_truetype.h
    ${IMGUI_INCLUDE_DIR}/backends/imgui_impl_opengl3.h
    ${IMGUI_INCLUDE_DIR}/backends/imgui_impl_glfw.h
  )

  set(IMGUI_SOURCES
    ${IMGUI_INCLUDE_DIR}/imgui.cpp
    ${IMGUI_INCLUDE_DIR}/imgui_demo.cpp
    ${IMGUI_INCLUDE_DIR}/imgui_draw.cpp
    ${IMGUI_INCLUDE_DIR}/imgui_tables.cpp
    ${IMGUI_INCLUDE_DIR}/imgui_widgets.cpp
    ${IMGUI_INCLUDE_DIR}/backends/imgui_impl_opengl3.cpp
    ${IMGUI_INCLUDE_DIR}/backends/imgui_impl_glfw.cpp
  )
endif()

mark_as_advanced(IMGUI_FOUND IMGUI_INCLUDE_DIR IMGUI_HEADERS, IMGUI_SOURCES)