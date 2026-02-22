/**
 * eng_render.h — はじむ用 2D レンダリングエンジン 公開 C API
 *
 * スタック: SDL2 (ウィンドウ/入力) + OpenGL 3.3 (描画)
 *           stb_image (テクスチャ) + stb_truetype (フォント)
 *
 * 使い方:
 *   ENG_Renderer* r = eng_create("ゲーム", 1280, 720);
 *   while (eng_update(r)) {
 *       eng_clear(r, 0.1f, 0.1f, 0.2f, 1.0f);
 *       eng_draw_sprite(r, tex, 100, 100, 64, 64);
 *       eng_flush(r);
 *   }
 *   eng_destroy(r);
 *
 * Copyright (c) 2026 Reo Shiozawa — MIT License
 */
#pragma once

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ── 型定義 ─────────────────────────────────────────────*/
typedef struct ENG_Renderer ENG_Renderer;
typedef uint32_t ENG_TexID;   /* 0 = 無効 */
typedef uint32_t ENG_FontID;  /* 0 = 無効 */

/* ── キーコード (SDL_Scancodeと対応) ────────────────────*/
typedef enum {
    ENG_KEY_UNKNOWN    = 0,
    ENG_KEY_A          = 4,
    ENG_KEY_B          = 5,
    ENG_KEY_C          = 6,
    ENG_KEY_D          = 7,
    ENG_KEY_E          = 8,
    ENG_KEY_F          = 9,
    ENG_KEY_G          = 10,
    ENG_KEY_H          = 11,
    ENG_KEY_I          = 12,
    ENG_KEY_J          = 13,
    ENG_KEY_K          = 14,
    ENG_KEY_L          = 15,
    ENG_KEY_M          = 16,
    ENG_KEY_N          = 17,
    ENG_KEY_O          = 18,
    ENG_KEY_P          = 19,
    ENG_KEY_Q          = 20,
    ENG_KEY_R          = 21,
    ENG_KEY_S          = 22,
    ENG_KEY_T          = 23,
    ENG_KEY_U          = 24,
    ENG_KEY_V          = 25,
    ENG_KEY_W          = 26,
    ENG_KEY_X          = 27,
    ENG_KEY_Y          = 28,
    ENG_KEY_Z          = 29,
    ENG_KEY_1          = 30,
    ENG_KEY_2          = 31,
    ENG_KEY_3          = 32,
    ENG_KEY_4          = 33,
    ENG_KEY_5          = 34,
    ENG_KEY_6          = 35,
    ENG_KEY_7          = 36,
    ENG_KEY_8          = 37,
    ENG_KEY_9          = 38,
    ENG_KEY_0          = 39,
    ENG_KEY_RETURN     = 40,
    ENG_KEY_ESCAPE     = 41,
    ENG_KEY_BACKSPACE  = 42,
    ENG_KEY_TAB        = 43,
    ENG_KEY_SPACE      = 44,
    ENG_KEY_UP         = 82,
    ENG_KEY_DOWN       = 81,
    ENG_KEY_LEFT       = 80,
    ENG_KEY_RIGHT      = 79,
    ENG_KEY_LSHIFT     = 225,
    ENG_KEY_RSHIFT     = 229,
    ENG_KEY_LCTRL      = 224,
    ENG_KEY_RCTRL      = 228,
    ENG_KEY_F1         = 58,
    ENG_KEY_F2         = 59,
    ENG_KEY_F3         = 60,
    ENG_KEY_F4         = 61,
    ENG_KEY_F5         = 62,
    ENG_KEY_F12        = 69,
} ENG_Key;

/* ── マウスボタン ───────────────────────────────────────*/
#define ENG_MOUSE_LEFT   1
#define ENG_MOUSE_MIDDLE 2
#define ENG_MOUSE_RIGHT  3

/* ── ライフサイクル ─────────────────────────────────────*/

/** ウィンドウ + OpenGL コンテキストを作成する */
ENG_Renderer* eng_create(const char* title, int width, int height);

/** イベントを処理し、ウィンドウが開いている間 true を返す */
bool          eng_update(ENG_Renderer* r);

/** ウィンドウとリソースを全て解放する */
void          eng_destroy(ENG_Renderer* r);

/** ウィンドウ幅を返す */
int           eng_width(ENG_Renderer* r);

/** ウィンドウ高さを返す */
int           eng_height(ENG_Renderer* r);

/** 経過時間 (秒) を返す */
double        eng_time(ENG_Renderer* r);

/** 直前フレームの delta time (秒) */
double        eng_delta(ENG_Renderer* r);

/** FPS */
double        eng_fps(ENG_Renderer* r);

/* ── 入力 ───────────────────────────────────────────────*/

/** キーが押されているか */
bool eng_key_down(ENG_Renderer* r, ENG_Key key);

/** キーがこのフレームで押されたか */
bool eng_key_pressed(ENG_Renderer* r, ENG_Key key);

/** キーがこのフレームで離されたか */
bool eng_key_released(ENG_Renderer* r, ENG_Key key);

/** マウスX座標 */
float eng_mouse_x(ENG_Renderer* r);

/** マウスY座標 */
float eng_mouse_y(ENG_Renderer* r);

/** マウスボタンが押されているか */
bool eng_mouse_down(ENG_Renderer* r, int button);

/** マウスボタンがこのフレームで押されたか */
bool eng_mouse_pressed(ENG_Renderer* r, int button);

/* ── 描画 (基本) ────────────────────────────────────────*/

/** 画面をクリア (RGBA 各 0.0〜1.0) */
void eng_clear(ENG_Renderer* r, float red, float green, float blue, float alpha);

/** バッチを GPU に送信してフレームを表示する */
void eng_flush(ENG_Renderer* r);

/* ── テクスチャ ─────────────────────────────────────────*/

/** 画像ファイルをロードして ID を返す (0 = エラー) */
ENG_TexID eng_load_texture(ENG_Renderer* r, const char* path);

/** ID を指定してテクスチャを解放する */
void      eng_free_texture(ENG_Renderer* r, ENG_TexID id);

/** テクスチャの幅 (px) */
int       eng_tex_width(ENG_Renderer* r, ENG_TexID id);

/** テクスチャの高さ (px) */
int       eng_tex_height(ENG_Renderer* r, ENG_TexID id);

/* ── スプライト描画 ─────────────────────────────────────*/

/** テクスチャ全体を (x,y) に (w,h) のサイズで描画 */
void eng_draw_sprite(ENG_Renderer* r, ENG_TexID id,
                     float x, float y, float w, float h);

/**
 * 拡張スプライト描画
 *   rot  — 回転角度 (度)
 *   ox,oy — 回転原点 (スプライト内 0.0〜1.0)
 *   cr,cg,cb,ca — 乗算カラー (0.0〜1.0)
 */
void eng_draw_sprite_ex(ENG_Renderer* r, ENG_TexID id,
                        float x, float y, float w, float h,
                        float rot, float ox, float oy,
                        float cr, float cg, float cb, float ca);

/**
 * UV 指定スプライト描画 (スプライトシート / アニメーション用)
 *   u0,v0 — UV 左上 (0.0〜1.0)
 *   u1,v1 — UV 右下 (0.0〜1.0)
 */
void eng_draw_sprite_uv(ENG_Renderer* r, ENG_TexID id,
                        float x,  float y,  float w,  float h,
                        float u0, float v0, float u1, float v1);

/** UV + 拡張描画 (スプライトシート + 回転 + 色) */
void eng_draw_sprite_uv_ex(ENG_Renderer* r, ENG_TexID id,
                           float x,  float y,  float w,  float h,
                           float u0, float v0, float u1, float v1,
                           float rot, float ox, float oy,
                           float cr, float cg, float cb, float ca);

/* ── 図形描画 ───────────────────────────────────────────*/

/** 矩形の輪郭を描画 */
void eng_draw_rect(ENG_Renderer* r,
                   float x, float y, float w, float h,
                   float cr, float cg, float cb, float ca);

/** 矩形を塗りつぶして描画 */
void eng_fill_rect(ENG_Renderer* r,
                   float x, float y, float w, float h,
                   float cr, float cg, float cb, float ca);

/** 円の輪郭を描画 */
void eng_draw_circle(ENG_Renderer* r,
                     float cx, float cy, float radius,
                     float cr, float cg, float cb, float ca);

/** 円を塗りつぶして描画 */
void eng_fill_circle(ENG_Renderer* r,
                     float cx, float cy, float radius,
                     float cr, float cg, float cb, float ca);

/** 直線を描画 */
void eng_draw_line(ENG_Renderer* r,
                   float x1, float y1, float x2, float y2,
                   float cr, float cg, float cb, float ca);

/* ── フォント / テキスト ────────────────────────────────*/

/** フォントファイル (.ttf/.otf) をロードして ID を返す */
ENG_FontID eng_load_font(ENG_Renderer* r, const char* path, float size);

/** システムデフォルトフォントをロード */
ENG_FontID eng_load_font_default(ENG_Renderer* r, float size);

/** フォントを解放する */
void       eng_free_font(ENG_Renderer* r, ENG_FontID id);

/** テキストを描画 */
void eng_draw_text(ENG_Renderer* r, ENG_FontID fid,
                   const char* text,
                   float x, float y,
                   float cr, float cg, float cb, float ca);

/** テキストの描画幅 (px) を計算 */
float eng_text_width(ENG_Renderer* r, ENG_FontID fid, const char* text);

/* ── カメラ ─────────────────────────────────────────────*/

/** カメラ位置を設定 */
void  eng_cam_pos(ENG_Renderer* r, float x, float y);

/** カメラのズーム倍率を設定 (1.0 = 等倍) */
void  eng_cam_zoom(ENG_Renderer* r, float zoom);

/** カメラの回転角を設定 (度) */
void  eng_cam_rot(ENG_Renderer* r, float degrees);

/** カメラをデフォルト (0,0 / zoom=1 / rot=0) にリセット */
void  eng_cam_reset(ENG_Renderer* r);

/** カメラのX座標を取得 */
float eng_cam_get_x(ENG_Renderer* r);

/** カメラのY座標を取得 */
float eng_cam_get_y(ENG_Renderer* r);

/** カメラのズームを取得 */
float eng_cam_get_zoom(ENG_Renderer* r);

/* ── ユーティリティ ─────────────────────────────────────*/

/** 乱数 (0.0〜1.0) */
float eng_randf(void);

/** 乱数整数 (min〜max) */
int   eng_randi(int min_val, int max_val);

#ifdef __cplusplus
} /* extern "C" */
#endif
