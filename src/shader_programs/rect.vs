#version 300 es
precision highp float; // for OpenGL 3.0 es

layout (location = 0) in vec2 aPosXY;
layout (location = 1) in float aPosZ;

uniform mat4 rotationMat;
uniform vec3 rgbColormap0;
uniform vec3 rgbColormap1;

out float height;
out vec3 rgb_colormap0;
out vec3 rgb_colormap1;

void main()
{
    float zScaling = 0.6;
    height = clamp(aPosZ / zScaling, 0.0, 1.0);

    // colormap base colors
    rgb_colormap0 = vec3(0.906,  1.000,  0.529);
    rgb_colormap1 = vec3(0.000,  0.502,  0.502);
    
    // rgb_colormap0 = rgbColormap0;
    // rgb_colormap1 = rgbColormap1;

    gl_Position = rotationMat * vec4(aPosXY.x, aPosXY.y, -aPosZ + zScaling/2.0, 1.0);
}