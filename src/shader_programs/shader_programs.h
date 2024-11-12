//===----------------------------------------------------------------------===//
//
// Shader program library for c-string shaders
//
// Note: updating .vert(or .vs) or .frag(or .fs) won't update the C files
// 
//===----------------------------------------------------------------------===//
#ifndef SHADER_PROGRAMS 
#define SHADER_PROGRAMS

#ifdef __cplusplus
extern "C" {
#endif


extern const char* vertexShader;

extern const char* fragmentShader;


#ifdef __cplusplus
}
#endif

#endif
