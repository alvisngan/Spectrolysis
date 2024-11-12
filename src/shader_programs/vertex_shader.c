#include "shader_programs.h"

const char* vertexShader = (
    "#version 300 es \n"
    "precision highp float; \n" // for OpenGL 3.0 es

    "layout (location = 0) in vec2 aPosXY; \n"
    "layout (location = 1) in float aPosZ; \n"

    "uniform mat4 rotationMat; \n"
    "uniform vec3 rgbColormap0; \n"
    "uniform vec3 rgbColormap1; \n"
    
    "out float height; \n"
    "out vec3 rgb_colormap0; \n"
    "out vec3 rgb_colormap1; \n"
    
    "void main() \n"
    "{ \n"
    "    float zScaling = 0.6; \n"
    "    height = clamp(aPosZ / zScaling, 0.0, 1.0); \n"

    // colormap base colors
    "    rgb_colormap0 = vec3(0.906,  1.000,  0.529); \n"
    "    rgb_colormap1 = vec3(0.000,  0.502,  0.502); \n"
    // rgb_colormap0 = rgbColormap0;
    // rgb_colormap1 = rgbColormap1;
    "   gl_Position = rotationMat * vec4(aPosXY.x, aPosXY.y, -aPosZ + zScaling/2.0, 1.0); \n"
    "}"
);