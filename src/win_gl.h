/**
 * win_gl.h — Windows OpenGL 2.0+ 関数ローダー
 *
 * SDL2/SDL_opengl.h と SDL2/SDL_opengl_glext.h のインクルード後に
 * このヘッダーをインクルードすること。
 * win_gl_load() は SDL_GL_CreateContext() 後に呼ぶこと。
 */
#pragma once

#ifdef _WIN32

/* ── 関数ポインタ extern 宣言 ──────────────────────────*/
extern PFNGLACTIVETEXTUREPROC            pfn_glActiveTexture;
extern PFNGLATTACHSHADERPROC             pfn_glAttachShader;
extern PFNGLBINDBUFFERPROC               pfn_glBindBuffer;
extern PFNGLBINDVERTEXARRAYPROC          pfn_glBindVertexArray;
extern PFNGLBUFFERDATAPROC               pfn_glBufferData;
extern PFNGLBUFFERSUBDATAPROC            pfn_glBufferSubData;
extern PFNGLCOMPILESHADERPROC            pfn_glCompileShader;
extern PFNGLCREATEPROGRAMPROC            pfn_glCreateProgram;
extern PFNGLCREATESHADERPROC             pfn_glCreateShader;
extern PFNGLDELETEBUFFERSPROC            pfn_glDeleteBuffers;
extern PFNGLDELETEPROGRAMPROC            pfn_glDeleteProgram;
extern PFNGLDELETESHADERPROC             pfn_glDeleteShader;
extern PFNGLDELETEVERTEXARRAYSPROC       pfn_glDeleteVertexArrays;
extern PFNGLENABLEVERTEXATTRIBARRAYPROC  pfn_glEnableVertexAttribArray;
extern PFNGLGENBUFFERSPROC               pfn_glGenBuffers;
extern PFNGLGENVERTEXARRAYSPROC          pfn_glGenVertexArrays;
extern PFNGLGETPROGRAMINFOLOGPROC        pfn_glGetProgramInfoLog;
extern PFNGLGETPROGRAMIVPROC             pfn_glGetProgramiv;
extern PFNGLGETSHADERINFOLOGPROC         pfn_glGetShaderInfoLog;
extern PFNGLGETSHADERIVPROC              pfn_glGetShaderiv;
extern PFNGLGETUNIFORMLOCATIONPROC       pfn_glGetUniformLocation;
extern PFNGLLINKPROGRAMPROC              pfn_glLinkProgram;
extern PFNGLSHADERSOURCEPROC             pfn_glShaderSource;
extern PFNGLUSEPROGRAMPROC               pfn_glUseProgram;
extern PFNGLVERTEXATTRIBPOINTERPROC      pfn_glVertexAttribPointer;
extern PFNGLUNIFORMMATRIX4FVPROC         pfn_glUniformMatrix4fv;
extern PFNGLUNIFORM1IPROC                pfn_glUniform1i;

/* ── gl* → pfn_gl* マクロ置換 ────────────────────────*/
#define glActiveTexture            pfn_glActiveTexture
#define glAttachShader             pfn_glAttachShader
#define glBindBuffer               pfn_glBindBuffer
#define glBindVertexArray          pfn_glBindVertexArray
#define glBufferData               pfn_glBufferData
#define glBufferSubData            pfn_glBufferSubData
#define glCompileShader            pfn_glCompileShader
#define glCreateProgram            pfn_glCreateProgram
#define glCreateShader             pfn_glCreateShader
#define glDeleteBuffers            pfn_glDeleteBuffers
#define glDeleteProgram            pfn_glDeleteProgram
#define glDeleteShader             pfn_glDeleteShader
#define glDeleteVertexArrays       pfn_glDeleteVertexArrays
#define glEnableVertexAttribArray  pfn_glEnableVertexAttribArray
#define glGenBuffers               pfn_glGenBuffers
#define glGenVertexArrays          pfn_glGenVertexArrays
#define glGetProgramInfoLog        pfn_glGetProgramInfoLog
#define glGetProgramiv             pfn_glGetProgramiv
#define glGetShaderInfoLog         pfn_glGetShaderInfoLog
#define glGetShaderiv              pfn_glGetShaderiv
#define glGetUniformLocation       pfn_glGetUniformLocation
#define glLinkProgram              pfn_glLinkProgram
#define glShaderSource             pfn_glShaderSource
#define glUseProgram               pfn_glUseProgram
#define glVertexAttribPointer      pfn_glVertexAttribPointer
#define glUniformMatrix4fv         pfn_glUniformMatrix4fv
#define glUniform1i                pfn_glUniform1i

/* ── ローダー関数 ─────────────────────────────────────*/
/** SDL_GL_CreateContext() 後に必ず呼ぶこと。
 *  戻り値: 成功=1, 失敗=0 */
int win_gl_load(void);

#endif /* _WIN32 */
