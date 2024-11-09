#ifndef GUI_COMPONENTS_HPP
#define GUI_COMPONENTS_HPP

#include <glad/glad.h>

#include "camera.hpp"
#include "audio_player.hpp"
#include "microphone.hpp"

/// Creating a widget or displaying an OpenGL viewport framebuffer texture
void guiViewport(Camera& camera, GLuint textureID);

/// Whether the cursor is on top of the viewport
bool guiViewportGetHovered();

/// Creating a widget for moth Microphone and Audioplayer
void guiAudioInterface(AudioPlayer& audioPlayer, Microphone& mic);
void guiAudioInterfaceCleanUp();

/// Switching between microphone and audioplayer
bool guiAudioInterfaceGetPlayerMode();

/// Menu for audio interface settings
void guiAudioInterfaceMenu();

/// Creat a amplitude v. frequency plot
void guiFrequencyPlot(float* x, float* y, int len, bool logScale = true);


#endif