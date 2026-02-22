/**
 * src/eng_window.c — SDL2 ウィンドウ + OpenGL コンテキスト管理
 */
#include "eng_internal.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>

/* ── ヘルパー: 投影行列の更新 ──────────────────────────*/
void eng_update_proj(ENG_Renderer* r) {
    /* 直交投影 + カメラ変換
     * 基底: (0,0) 左上、(win_w, win_h) 右下
     * カメラ: 平行移動 + ズーム + 回転
     */
    float W  = (float)r->win_w;
    float H  = (float)r->win_h;
    float cx = r->cam_x;
    float cy = r->cam_y;
    float z  = r->cam_zoom;
    float a  = r->cam_rot_deg * (float)(3.14159265358979323846 / 180.0);
    float ca = cosf(a), sa = sinf(a);

    /* ortho(-W/2, W/2, H/2, -H/2) → view space
     * +カメラ変換 (転置してから構築)
     * 列優先行列 (OpenGL標準) */
    float sx = 2.0f / W, sy = -2.0f / H;
    /* スクリーン→NDC の正射影:
     *   x' = (x - W/2) * 2/W
     *   y' = (y - H/2) * -2/H  (Y反転)
     * カメラ適用後:
     *   xw = (x - cx) * z
     * → x' = (xw - W/2) * 2/W
     */
    /* combined ortho * camera */
    /* column-major: proj[col][row] */
    float proj[16];
    memset(proj, 0, sizeof(proj));
    /* scale by zoom and ortho */
    float rsx = sx * z * ca;
    float rsy = sx * z * sa;
    float rtx = sx * z * (-cx * ca - cy * sa) - 1.0f;
    float rsx2 = sy * z * (-sa);
    float rsy2 = sy * z * ca;
    float rty  = sy * z * (cx * sa - cy * ca) + 1.0f;

    proj[0]  = rsx;   proj[1]  = rsx2; proj[2]  = 0; proj[3]  = 0;
    proj[4]  = rsy;   proj[5]  = rsy2; proj[6]  = 0; proj[7]  = 0;
    proj[8]  = 0;     proj[9]  = 0;    proj[10] = 1; proj[11] = 0;
    proj[12] = rtx;   proj[13] = rty;  proj[14] = 0; proj[15] = 1;

    glUseProgram(r->batch.prog);
    glUniformMatrix4fv(r->batch.loc_proj, 1, GL_FALSE, proj);
}

/* ── 生成 ───────────────────────────────────────────────*/
ENG_Renderer* eng_create(const char* title, int width, int height) {
    /* SDL 初期化 */
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS | SDL_INIT_TIMER) != 0) {
        fprintf(stderr, "[eng_render] SDL_Init: %s\n", SDL_GetError());
        return NULL;
    }

    /* OpenGL 3.3 Core Profile */
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 0);   /* 2D: 深度バッファ不要 */

    SDL_Window* win = SDL_CreateWindow(
        title,
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        width, height,
        SDL_WINDOW_OPENGL | SDL_WINDOW_ALLOW_HIGHDPI
    );
    if (!win) {
        fprintf(stderr, "[eng_render] SDL_CreateWindow: %s\n", SDL_GetError());
        SDL_Quit();
        return NULL;
    }

    SDL_GLContext ctx = SDL_GL_CreateContext(win);
    if (!ctx) {
        fprintf(stderr, "[eng_render] SDL_GL_CreateContext: %s\n", SDL_GetError());
        SDL_DestroyWindow(win);
        SDL_Quit();
        return NULL;
    }
    SDL_GL_MakeCurrent(win, ctx);
    SDL_GL_SetSwapInterval(1);  /* VSync */

    /* 実際のサイズ取得 (Retina対応) */
    int dw, dh;
    SDL_GL_GetDrawableSize(win, &dw, &dh);

    glViewport(0, 0, dw, dh);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    ENG_Renderer* r = (ENG_Renderer*)calloc(1, sizeof(ENG_Renderer));
    if (!r) {
        SDL_GL_DeleteContext(ctx);
        SDL_DestroyWindow(win);
        SDL_Quit();
        return NULL;
    }

    r->window  = win;
    r->gl_ctx  = ctx;
    r->win_w   = dw;
    r->win_h   = dh;
    r->cam_zoom = 1.0f;

    /* タイミング初期化 */
    r->start_tick = SDL_GetPerformanceCounter();
    r->prev_tick  = r->start_tick;
    r->fps        = 60.0;

    /* キー入力初期化 */
    r->key_state = SDL_GetKeyboardState(NULL);
    memset(r->key_prev, 0, sizeof(r->key_prev));

    /* シェーダー + バッチ初期化 */
    if (!eng_shader_init(&r->batch)) {
        free(r);
        SDL_GL_DeleteContext(ctx);
        SDL_DestroyWindow(win);
        SDL_Quit();
        return NULL;
    }
    eng_batch_init(&r->batch, r->batch.prog);

    /* 初期投影行列 */
    eng_update_proj(r);

    /* 乱数初期化 */
    srand((unsigned)time(NULL));

    fprintf(stderr, "[eng_render] 初期化完了 %dx%d (drawable: %dx%d)\n",
            width, height, dw, dh);
    return r;
}

/* ── イベント処理 ───────────────────────────────────────*/
bool eng_update(ENG_Renderer* r) {
    if (!r || r->quit_requested) return false;

    /* delta time */
    uint64_t now   = SDL_GetPerformanceCounter();
    uint64_t freq  = SDL_GetPerformanceFrequency();
    r->delta       = (double)(now - r->prev_tick) / (double)freq;
    if (r->delta > 0.1) r->delta = 0.1; /* 最大 0.1s */
    r->fps         = (r->delta > 0.0) ? 1.0 / r->delta : 60.0;
    r->prev_tick   = now;

    /* キー状態を前フレームに保存 */
    memcpy(r->key_prev, r->key_state, 512);
    r->mouse_prev  = r->mouse_state;
    r->mouse_wheel = 0.0f;  /* ホイールはイベントベースでリセット */

    /* イベント処理 */
    SDL_Event ev;
    while (SDL_PollEvent(&ev)) {
        if (ev.type == SDL_QUIT) {
            r->quit_requested = true;
            return false;
        }
        if (ev.type == SDL_WINDOWEVENT &&
            ev.window.event == SDL_WINDOWEVENT_RESIZED) {
            SDL_GL_GetDrawableSize(r->window, &r->win_w, &r->win_h);
            glViewport(0, 0, r->win_w, r->win_h);
            eng_update_proj(r);
        }
        if (ev.type == SDL_MOUSEWHEEL) {
            r->mouse_wheel += (float)ev.wheel.y;
        }
    }

    /* マウス状態更新 */
    int mx, my;
    r->mouse_state = SDL_GetMouseState(&mx, &my);
    r->mouse_x = (float)mx;
    r->mouse_y = (float)my;

    /* FPS キャップ */
    if (r->fps_cap > 0) {
        uint64_t freq     = SDL_GetPerformanceFrequency();
        uint64_t target   = freq / (uint64_t)r->fps_cap;
        uint64_t elapsed  = SDL_GetPerformanceCounter() - r->frame_end_tick;
        if (elapsed < target) {
            uint32_t wait_ms = (uint32_t)(((target - elapsed) * 1000) / freq);
            if (wait_ms > 0) SDL_Delay(wait_ms);
        }
    }
    r->frame_end_tick = SDL_GetPerformanceCounter();

    return true;
}

/* ── 解放 ───────────────────────────────────────────────*/
void eng_destroy(ENG_Renderer* r) {
    if (!r) return;

    /* テクスチャ解放 */
    for (int i = 0; i < ENG_MAX_TEXTURES; ++i) {
        if (r->textures[i].used) {
            glDeleteTextures(1, &r->textures[i].gl_id);
        }
    }

    /* フォント解放 */
    for (int i = 0; i < ENG_MAX_FONTS; ++i) {
        if (r->fonts[i].used) {
            glDeleteTextures(1, &r->fonts[i].atlas_tex);
            free(r->fonts[i].baked);
        }
    }

    /* バッチ解放 */
    if (r->batch.vao) glDeleteVertexArrays(1, &r->batch.vao);
    if (r->batch.vbo) glDeleteBuffers(1, &r->batch.vbo);
    if (r->batch.ibo) glDeleteBuffers(1, &r->batch.ibo);
    if (r->batch.prog) glDeleteProgram(r->batch.prog);

    SDL_GL_DeleteContext(r->gl_ctx);
    SDL_DestroyWindow(r->window);
    SDL_Quit();
    free(r);
}

/* ── クエリ ─────────────────────────────────────────────*/
int    eng_width(ENG_Renderer* r)  { return r ? r->win_w : 0; }
int    eng_height(ENG_Renderer* r) { return r ? r->win_h : 0; }
double eng_time(ENG_Renderer* r) {
    if (!r) return 0.0;
    uint64_t freq = SDL_GetPerformanceFrequency();
    return (double)(SDL_GetPerformanceCounter() - r->start_tick) / (double)freq;
}
double eng_delta(ENG_Renderer* r) { return r ? r->delta : 0.0; }
double eng_fps(ENG_Renderer* r)   { return r ? r->fps   : 0.0; }

/* ── 入力 ───────────────────────────────────────────────*/
bool  eng_key_down(ENG_Renderer* r, ENG_Key key) {
    return r && (int)key < 512 && r->key_state[(int)key];
}
bool  eng_key_pressed(ENG_Renderer* r, ENG_Key key) {
    return r && (int)key < 512 && r->key_state[(int)key] && !r->key_prev[(int)key];
}
bool  eng_key_released(ENG_Renderer* r, ENG_Key key) {
    return r && (int)key < 512 && !r->key_state[(int)key] && r->key_prev[(int)key];
}
float eng_mouse_x(ENG_Renderer* r)            { return r ? r->mouse_x : 0.0f; }
float eng_mouse_y(ENG_Renderer* r)            { return r ? r->mouse_y : 0.0f; }
bool  eng_mouse_down(ENG_Renderer* r, int b)  { return r && (r->mouse_state & SDL_BUTTON(b)); }
bool  eng_mouse_pressed(ENG_Renderer* r, int b) {
    return r && (r->mouse_state & SDL_BUTTON(b)) && !(r->mouse_prev & SDL_BUTTON(b));
}
bool  eng_mouse_released(ENG_Renderer* r, int b) {
    return r && !(r->mouse_state & SDL_BUTTON(b)) && (r->mouse_prev & SDL_BUTTON(b));
}
float eng_mouse_wheel(ENG_Renderer* r) { return r ? r->mouse_wheel : 0.0f; }

/* ── FPS キャップ ───────────────────────────────────────*/
void eng_set_fps_cap(ENG_Renderer* r, int fps) {
    if (r) {
        r->fps_cap = fps;
        r->frame_end_tick = SDL_GetPerformanceCounter();
    }
}

/* ── ユーティリティ ─────────────────────────────────────*/
float eng_randf(void) { return (float)rand() / (float)RAND_MAX; }
int   eng_randi(int mn, int mx) {
    if (mn >= mx) return mn;
    return mn + rand() % (mx - mn + 1);
}
/* ── フルスクリーン / カーソル ──────────────────────────────────────*/
void eng_set_fullscreen(ENG_Renderer* r, bool fullscreen) {
    if (!r) return;
    Uint32 flags = fullscreen ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0;
    SDL_SetWindowFullscreen(r->window, flags);
    SDL_GetWindowSize(r->window, &r->win_w, &r->win_h);
    glViewport(0, 0, r->win_w, r->win_h);
    eng_update_proj(r);
}

void eng_set_cursor_visible(ENG_Renderer* r, bool visible) {
    (void)r;
    SDL_ShowCursor(visible ? SDL_ENABLE : SDL_DISABLE);
}