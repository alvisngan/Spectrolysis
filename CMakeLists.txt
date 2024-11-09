cmake_minimum_required(VERSION 3.14)
project(Spectrolysis)

# Find required packages
find_package(OpenGL REQUIRED)
find_package(SDL2 REQUIRED)
include_directories(${SDL2_INCLUDE_DIRS})

## EXTERNAL

# Add the external directory for the project
include_directories(${PROJECT_SOURCE_DIR}/external)

# Add glad library
add_library(glad external/glad/src/glad.c)
target_include_directories(glad PUBLIC external/glad/include)

# Add imgui library
add_library(imgui
  external/imgui/imgui.cpp 
  # external/imgui/imgui_demo.cpp 
  external/imgui/imgui_draw.cpp 
  external/imgui/imgui_tables.cpp 
  external/imgui/imgui_widgets.cpp 
  external/imgui/backends/imgui_impl_sdl2.cpp 
  external/imgui/backends/imgui_impl_opengl3.cpp
)
target_include_directories(imgui PUBLIC external/imgui external/imgui/backends)

# Add implot library
add_library(implot
  # external/implot/implot_demo.cpp 
  external/implot/implot_items.cpp 
  external/implot/implot.cpp 
)
target_link_libraries(implot PUBLIC imgui)
target_include_directories(implot PUBLIC external/implot external/implot)

# Add pffft library
add_library(pffft external/pffft/pffft.c)
target_include_directories(pffft PUBLIC external/pffft)

# Add tinyfiledialogs library
add_library(tinyfiledialogs external/tinyfiledialogs/tinyfiledialogs.c)
target_include_directories(tinyfiledialogs PUBLIC external/tinyfiledialogs)

## SRC
add_library(array2d STATIC src/array2d.cpp)

# Add shader library
add_library(shader src/shader.cpp)
target_link_libraries(shader PRIVATE glad)

add_library(grid src/grid.cpp)
target_link_libraries(grid PRIVATE glad array2d)

# Add camera library
add_library(camera src/camera.cpp)
target_include_directories(tinyfiledialogs PUBLIC external/glm)
target_link_libraries(camera PRIVATE)

# Add frame buffer library
add_library(frame_buffer src/frame_buffer.cpp)
target_link_libraries(frame_buffer PRIVATE glad)

# Add audio_player library
add_library(audio_player src/audio_player.cpp)
target_link_libraries(audio_player PRIVATE SDL2)

# Add microphone library
add_library(microphone src/microphone.cpp)
target_link_libraries(microphone PRIVATE SDL2)

# Add fft library
add_library(fft src/fft.cpp)
target_link_libraries(fft PRIVATE pffft)

# Add smooth library
add_library(smoothing src/smoothing.cpp)
target_link_libraries(smoothing PRIVATE pffft array2d)
if(OpenMP_CXX_FOUND)
  target_link_libraries(smoothing PRIVATE OpenMP::OpenMP_CXX)
endif()

# Add file dialog library
add_library(file_dialog src/file_dialog.cpp)
target_link_libraries(file_dialog PRIVATE tinyfiledialogs)

# Add gui library
add_library(gui_components 
  src/gui/gui_audio_interface.cpp 
  src/gui/gui_frequency_plot.cpp 
  src/gui/gui_viewport.cpp
)
target_include_directories(gui_components PRIVATE src)
target_link_libraries(gui_components PRIVATE glad imgui implot file_dialog)

add_library(gui_theme src/gui/gui_theme.cpp)
target_link_libraries(gui_theme PRIVATE imgui implot)

add_library(gui src/gui/gui.cpp)
target_include_directories(gui PUBLIC src src/gui)  # has to be PUBLIC, main needs to use this
target_link_libraries(gui PRIVATE glad imgui implot gui_components gui_theme)

# Main executable 
add_executable(main src/main.cpp)
target_link_libraries(main PRIVATE 
  SDL2 
  glad 
  imgui 
  implot 
  array2d
  shader 
  grid 
  camera 
  frame_buffer 
  gui
  audio_player 
  microphone
  fft
  smoothing
)

# Add compiler-specific options
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
  target_compile_options(main PRIVATE -pedantic-errors -Wall -Wextra)
elseif(MSVC)
  target_compile_options(main PRIVATE /W4 /permissive-)
endif()

add_subdirectory(test)
