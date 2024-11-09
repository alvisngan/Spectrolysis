//===----------------------------------------------------------------------===//
//
// GUI library with Dear ImGui
// 
//===----------------------------------------------------------------------===//
#ifndef GUI_HPP
#define GUI_HPP

#include <SDL2/SDL.h>
// #include <SDL2/SDL_opengl.h>
#include <glad/glad.h>

#include "camera.hpp"
#include "audio_player.hpp"
#include "microphone.hpp"
#include "grid.hpp"

typedef struct {
    int freqPlotLen;
    float* freqPlotX;
    float* freqPlotY;
    GLuint viewportTextureID;
    Camera* cameraPtr;         // Use pointers
    AudioPlayer* audioPlayerPtr;
    Microphone* micPtr;
    Grid* gridPtr;
} guiInputs;

/// \param version GLSL version
void guiInit(SDL_Window *window, 
             SDL_GLContext gl_context, 
             const char* version = "#version 100");

void guiNewFrame();

void guiApp(guiInputs& inputs);

void guiRender();

void guiCleanUp();


// forward declaration from gui_components.hpp
// -------------------------------------------
/// Whether the cursor is on top of the viewport
bool guiViewportGetHovered();

/// Switching between microphone and audioplayer
bool guiAudioInterfaceGetPlayerMode();




#endif