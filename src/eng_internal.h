/**
 * eng_internal.h — エンジン内部構造体定義
 * src/ からのみインクルードする
 */
#pragma once

#include "eng_render.h"

#ifdef __APPLE__
  #define GL_SILENCE_DEPRECATION
  #include <OpenGL/gl3.h>
#elif defined(__linux__)
  #define GL_GLEXT_PROTOTYPES
  #include <GL/gl.h>
  #include <GL/glext.h>
#endif

#include <SDL2/SDL.h>
#include <stdint.h>
#include <stdbool.h>
#include <math.h>

/* ── スプライトバッチ ───────────────────────────────────*/
#define ENG_MAX_BATCH   4096   /* 1バッチ最大スプライト数 */
#define ENG_MAX_TEXTURES 1024
#define ENG_MAX_FONTS      16

typedef struct {
    float x, y;   /* 位置 */
    float u, v;   /* UV */
    float r, g, b, a; /* 色 */
} ENG_Vertex;  /* 32 bytes */

typedef struct {
    GLuint  vao, vbo, ibo;
    ENG_Vertex verts[ENG_MAX_BATCH * 4];
    int     quad_count;
    GLuint  current_tex;  /* 現在バインド中のテクスチャ */
    bool    use_tex;
    GLuint  prog;         /* シェーダープログラム */
    GLint   loc_proj;
    GLint   loc_use_tex;
    GLint   loc_tex;
} ENG_Batch;

/* ── テクスチャエントリ ─────────────────────────────────*/
typedef struct {
    GLuint gl_id;
    int    w, h;
    bool   used;
} ENG_TexEntry;

/* ── フォントエントリ ───────────────────────────────────*/
#define ENG_FONT_ATLAS_W 512
#define ENG_FONT_ATLAS_H 512

typedef struct {
    GLuint  atlas_tex;
    float   size;
    bool    used;
    /* stb_truetype baked chars: ASCII 32〜127 */
    void*   baked; /* stbtt_bakedchar[96] */
} ENG_FontEntry;

/* ── メインレンダラー構造体 ────────────────────────────*/
struct ENG_Renderer {
    /* SDL */
    SDL_Window*   window;
    SDL_GLContext gl_ctx;
    int           win_w, win_h;

    /* タイミング */
    uint64_t start_tick;
    uint64_t prev_tick;
    double   delta;
    double   fps;

    /* 入力 */
    const uint8_t* key_state;     /* SDL_GetKeyboardState */
    uint8_t  key_prev[512];
    float    mouse_x, mouse_y;
    uint32_t mouse_state;         /* SDL_GetMouseState */
    uint32_t mouse_prev;
    float    mouse_wheel;         /* このフレームのホイール量 */
    bool     quit_requested;

    /* FPS キャップ */
    int      fps_cap;             /* 0=無制限 */
    uint64_t frame_end_tick;      /* 前フレーム終了時刻 */

    /* クリッピング矩形 */
    bool     clip_active;
    int      clip_x, clip_y, clip_w, clip_h;

    /* カメラ */
    float cam_x, cam_y, cam_zoom, cam_rot_deg;

    /* バッチ */
    ENG_Batch batch;

    /* テクスチャプール */
    ENG_TexEntry textures[ENG_MAX_TEXTURES];
    int          tex_count;

    /* フォントプール */
    ENG_FontEntry fonts[ENG_MAX_FONTS];
    int           font_count;
};

/* ── 内部関数 ───────────────────────────────────────────*/
bool eng_shader_init(ENG_Batch* b);
void eng_batch_init(ENG_Batch* b, GLuint prog);
void eng_batch_flush(ENG_Renderer* r);
void eng_batch_push_quad(ENG_Renderer* r,
    float x, float y, float w, float h,
    float u0, float v0, float u1, float v1,
    float rot, float ox, float oy,
    float cr, float cg, float cb, float ca,
    GLuint tex, bool use_tex);
void eng_update_proj(ENG_Renderer* r);
GLuint eng_tex_gl_id(ENG_Renderer* r, ENG_TexID id);
