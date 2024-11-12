#include "shader_programs.h"

const char* fragmentShader = (
    "#version 300 es \n"
    "precision highp float; \n" // for OpenGL 3.0 es

    "out vec4 FragColor; \n"

    "in float height; \n"
    "in vec3 rgb_colormap0; \n"
    "in vec3 rgb_colormap1; \n"

    "vec3 colormap(float height); \n"
    "vec3 srgb_to_oklab(vec3 rgb); \n"
    "vec3 oklab_to_srgb(vec3 lab); \n"
    
    "void main() \n"
    "{ \n"
        // provide some depth based lighting
    "    float depth_contribution = 0.8; \n" // [0.0, 1.0]
    "    vec3 inverted_depth = 1.0 - vec3(gl_FragCoord.z) * depth_contribution; \n"

        // colormap
    "    vec3 color = colormap(height); \n" // Blue to Red based on height

        // combining colormap with depth
    "    color *= inverted_depth; \n"

    "    FragColor = vec4(color, 1.0); \n"
    "} \n"

    "vec3 colormap(float height) \n"
    "{ \n"
        // convert to OKLAB before interpolate
    "    vec3 oklab_colormap0 = srgb_to_oklab(rgb_colormap0); \n"
    "    vec3 oklab_colormap1 = srgb_to_oklab(rgb_colormap1); \n"

        // creat colormap spectrum with interpolation in OKLAB
        // since height is clamp between [0, 1]
    "    vec3 oklab_colormap =   height * oklab_colormap0 \n"
    "                         + (1.0 - height) * oklab_colormap1; \n"

        // convert back to RGB and return
    "    return oklab_to_srgb(oklab_colormap); \n"
    "} \n"


    /**
     * GLSL matrix order (column major order):
     * 
     *  mat3 m = mat3(m11, m21, m31, 
     *                m12, m22, m32,
     *                m13, m23, m33)
     *
     *
     *  srgb_to_oklab m1, in traditional representation (math style):
     *
     *   0.4122214708,  0.5363325363,  0.0514459929,
     *	 0.2119034982,  0.6806995451,  0.1073969566,
     *	 0.0883024619,  0.2817188376,  0.6299787005,
     *
     *  simimlarly for m2:
     *
     *   0.2104542553,  0.7936177850, -0.0040720468,
     *   1.9779984951, -2.4285922050,  0.4505937099,
     *   0.0259040371,  0.7827717662, -0.8086757660,
     */
    "vec3 srgb_to_oklab(vec3 rgb) \n"
    "{ \n"
    "    mat3 m1 = mat3 ( \n"
    "         0.4122214708,  0.2119034982,  0.0883024619, \n"
    "         0.5363325363,  0.6806995451,  0.2817188376, \n"
    "         0.0514459929,  0.1073969566,  0.6299787005 \n"
    "    ); \n"
    
    "    mat3 m2 = mat3 ( \n"
    "         0.2104542553,  1.9779984951,  0.0259040371, \n"
    "         0.7936177850, -2.4285922050,  0.7827717662, \n"
    "        -0.0040720468,  0.4505937099, -0.8086757660 \n"
    "    ); \n"
    
    "    vec3 lms = m1 * rgb; \n"
    "    vec3 lms_ = pow(lms, vec3(0.333333333)); \n"
    
    "    return m2 * lms_; \n"
    "} \n"

    /**
     * GLSL matrix order (column major order):
     * 
     *  mat3 m = mat3(m11, m21, m31, 
     *                m12, m22, m32,
     *                m13, m23, m33)
     *
     *
     *  oklab_to_srgb m2_, in traditional representation (math style):
     *
     *   1.0000000000,  0.3963377774,  0.2158037573,
     *   1.0000000000, -0.1055613458, -0.0638541728,
     *   1.0000000000, -0.0894841775, -1.2914855480,
     *
     *  simimlarly for m1_:
     *
     *   4.0767416621, -3.3077115913,  0.2309699292,
     *  -1.2684380046,  2.6097574011, -0.3413193965,
     *  -0.0041960863, -0.7034186147,  1.7076147010,
     */
    "vec3 oklab_to_srgb(vec3 lab) \n"
    "{ \n"
    "    mat3 m2_ = mat3 ( \n"
    "         1.0000000000,  1.0000000000,  1.0000000000, \n"
    "         0.3963377774, -0.1055613458, -0.0894841775, \n"
    "         0.2158037573, -0.0638541728, -1.2914855480 \n"
    "    ); \n"

    "    mat3 m1_ = mat3 ( \n"
    "         4.0767416621, -1.2684380046, -0.0041960863, \n"
    "        -3.3077115913,  2.6097574011, -0.7034186147, \n"
    "         0.2309699292, -0.3413193965,  1.7076147010 \n"
    "    ); \n"

    "    vec3 lms_ = m2_ * lab; \n"
    "    vec3 lms = lms_ * lms_ * lms_; \n"

    "    return m1_ * lms; \n"
    "}"
);