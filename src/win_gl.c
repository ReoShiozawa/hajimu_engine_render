/**
 * win_gl.c — Windows OpenGL 2.0+ 関数ローダー実装
 *
 * SDL_GL_GetProcAddress() を使って GL 拡張関数を動的にロードする。
 * SDL_GL_CreateContext() 後に win_gl_load() を呼ぶこと。
 */
#ifdef _WIN32

#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>
#include <SDL2/SDL_opengl_glext.h>
#include "win_gl.h"

/* ── 関数ポインタ実体 ────────────────────────────────*/
PFNGLACTIVETEXTUREPROC            pfn_glActiveTexture;
PFNGLATTACHSHADERPROC             pfn_glAttachShader;
PFNGLBINDBUFFERPROC               pfn_glBindBuffer;
PFNGLBINDVERTEXARRAYPROC          pfn_glBindVertexArray;
PFNGLBUFFERDATAPROC               pfn_glBufferData;
PFNGLBUFFERSUBDATAPROC            pfn_glBufferSubData;
PFNGLCOMPILESHADERPROC            pfn_glCompileShader;
PFNGLCREATEPROGRAMPROC            pfn_glCreateProgram;
PFNGLCREATESHADERPROC             pfn_glCreateShader;
PFNGLDELETEBUFFERSPROC            pfn_glDeleteBuffers;
PFNGLDELETEPROGRAMPROC            pfn_glDeleteProgram;
PFNGLDELETESHADERPROC             pfn_glDeleteShader;
PFNGLDELETEVERTEXARRAYSPROC       pfn_glDeleteVertexArrays;
PFNGLENABLEVERTEXATTRIBARRAYPROC  pfn_glEnableVertexAttribArray;
PFNGLGENBUFFERSPROC               pfn_glGenBuffers;
PFNGLGENVERTEXARRAYSPROC          pfn_glGenVertexArrays;
PFNGLGETPROGRAMINFOLOGPROC        pfn_glGetProgramInfoLog;
PFNGLGETPROGRAMIVPROC             pfn_glGetProgramiv;
PFNGLGETSHADERINFOLOGPROC         pfn_glGetShaderInfoLog;
PFNGLGETSHADERIVPROC              pfn_glGetShaderiv;
PFNGLGETUNIFORMLOCATIONPROC       pfn_glGetUniformLocation;
PFNGLLINKPROGRAMPROC              pfn_glLinkProgram;
PFNGLSHADERSOURCEPROC             pfn_glShaderSource;
PFNGLUSEPROGRAMPROC               pfn_glUseProgram;
PFNGLVERTEXATTRIBPOINTERPROC      pfn_glVertexAttribPointer;
PFNGLUNIFORMMATRIX4FVPROC         pfn_glUniformMatrix4fv;
PFNGLUNIFORM1IPROC                pfn_glUniform1i;

/* ── ローダー ──────────────────────────────────────*/
#define LOAD(var, name) \
    var = (typeof(var)) SDL_GL_GetProcAddress(name); \
    if (!var) { \
        SDL_Log("[win_gl] WARNING: %s not found\n", name); \
    }

int win_gl_load(void) {
    LOAD(pfn_glActiveTexture,           "glActiveTexture")
    LOAD(pfn_glAttachShader,            "glAttachShader")
    LOAD(pfn_glBindBuffer,              "glBindBuffer")
    LOAD(pfn_glBindVertexArray,         "glBindVertexArray")
    LOAD(pfn_glBufferData,              "glBufferData")
    LOAD(pfn_glBufferSubData,           "glBufferSubData")
    LOAD(pfn_glCompileShader,           "glCompileShader")
    LOAD(pfn_glCreateProgram,           "glCreateProgram")
    LOAD(pfn_glCreateShader,            "glCreateShader")
    LOAD(pfn_glDeleteBuffers,           "glDeleteBuffers")
    LOAD(pfn_glDeleteProgram,           "glDeleteProgram")
    LOAD(pfn_glDeleteShader,            "glDeleteShader")
    LOAD(pfn_glDeleteVertexArrays,      "glDeleteVertexArrays")
    LOAD(pfn_glEnableVertexAttribArray, "glEnableVertexAttribArray")
    LOAD(pfn_glGenBuffers,              "glGenBuffers")
    LOAD(pfn_glGenVertexArrays,         "glGenVertexArrays")
    LOAD(pfn_glGetProgramInfoLog,       "glGetProgramInfoLog")
    LOAD(pfn_glGetProgramiv,            "glGetProgramiv")
    LOAD(pfn_glGetShaderInfoLog,        "glGetShaderInfoLog")
    LOAD(pfn_glGetShaderiv,             "glGetShaderiv")
    LOAD(pfn_glGetUniformLocation,      "glGetUniformLocation")
    LOAD(pfn_glLinkProgram,             "glLinkProgram")
    LOAD(pfn_glShaderSource,            "glShaderSource")
    LOAD(pfn_glUseProgram,              "glUseProgram")
    LOAD(pfn_glVertexAttribPointer,     "glVertexAttribPointer")
    LOAD(pfn_glUniformMatrix4fv,        "glUniformMatrix4fv")
    LOAD(pfn_glUniform1i,               "glUniform1i")
    return 1;
}

#endif /* _WIN32 */
