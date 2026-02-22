/**
 * src/plugin.c — はじむ 2D レンダリングエンジン プラグインエントリー
 *
 * Copyright (c) 2026 Reo Shiozawa — MIT License
 */
#include "hajimu_plugin.h"
#include "eng_render.h"
#include <string.h>
#include <stdio.h>

static ENG_Renderer* g_r = NULL;

/* ── ヘルパーマクロ ─────────────────────────────────────*/
#define ARG_NUM(i) ((i) < argc && args[(i)].type == VALUE_NUMBER ? args[(i)].number : 0.0)
#define ARG_STR(i) ((i) < argc && args[(i)].type == VALUE_STRING  ? args[(i)].string.data : "")
#define ARG_INT(i) ((int)ARG_NUM(i))
#define ARG_F(i)   ((float)ARG_NUM(i))
#define ARG_B(i)   ((i) < argc && args[(i)].type == VALUE_BOOL ? args[(i)].boolean : false)
#define NUM(v)     hajimu_number((double)(v))
#define BVAL(v)    hajimu_bool((bool)(v))
#define NUL        hajimu_null()

/* ================================================================
 * ライフサイクル
 * ================================================================ */
static Value fn_ウィンドウ作成(int argc, Value* args) {
    if (g_r) { eng_destroy(g_r); g_r = NULL; }
    g_r = eng_create(ARG_STR(0), ARG_INT(1), ARG_INT(2));
    return NUM(g_r ? 0 : -1);
}
static Value fn_ウィンドウ削除(int argc, Value* args) {
    (void)argc; (void)args;
    if (g_r) { eng_destroy(g_r); g_r = NULL; }
    return NUL;
}
static Value fn_ウィンドウ更新(int argc, Value* args) {
    (void)argc; (void)args;
    return BVAL(g_r && eng_update(g_r));
}
static Value fn_ウィンドウ幅(int argc, Value* args)  { (void)argc; (void)args; return NUM(eng_width(g_r)); }
static Value fn_ウィンドウ高さ(int argc, Value* args) { (void)argc; (void)args; return NUM(eng_height(g_r)); }
static Value fn_経過時間(int argc, Value* args)       { (void)argc; (void)args; return NUM(eng_time(g_r)); }
static Value fn_デルタ時間(int argc, Value* args)    { (void)argc; (void)args; return NUM(eng_delta(g_r)); }
static Value fn_FPS(int argc, Value* args)            { (void)argc; (void)args; return NUM(eng_fps(g_r)); }

/* ================================================================
 * 入力
 * ================================================================ */
static Value fn_キー押下中(int argc, Value* args)       { return BVAL(eng_key_down(g_r, (ENG_Key)ARG_INT(0))); }
static Value fn_キー押下(int argc, Value* args)         { return BVAL(eng_key_pressed(g_r, (ENG_Key)ARG_INT(0))); }
static Value fn_キー離した(int argc, Value* args)       { return BVAL(eng_key_released(g_r, (ENG_Key)ARG_INT(0))); }
static Value fn_マウスX(int argc, Value* args)          { (void)argc; (void)args; return NUM(eng_mouse_x(g_r)); }
static Value fn_マウスY(int argc, Value* args)          { (void)argc; (void)args; return NUM(eng_mouse_y(g_r)); }
static Value fn_マウスボタン押下中(int argc, Value* args) { return BVAL(eng_mouse_down(g_r, ARG_INT(0))); }
static Value fn_マウスボタン押下(int argc, Value* args)  { return BVAL(eng_mouse_pressed(g_r, ARG_INT(0))); }
static Value fn_マウスボタン離した(int argc, Value* args) { return BVAL(eng_mouse_released(g_r, ARG_INT(0))); }
static Value fn_マウスホイール(int argc, Value* args)    { (void)argc; (void)args; return NUM(eng_mouse_wheel(g_r)); }

/* FPS キャップ */
static Value fn_FPS上限設定(int argc, Value* args) { eng_set_fps_cap(g_r, ARG_INT(0)); return hajimu_null(); }

/* クリッピング */
static Value fn_クリップ開始(int argc, Value* args) { eng_clip_begin(g_r, ARG_F(0), ARG_F(1), ARG_F(2), ARG_F(3)); return hajimu_null(); }
static Value fn_クリップ終了(int argc, Value* args) { (void)argc; (void)args; eng_clip_end(g_r); return hajimu_null(); }

static Value fn_キーコード(int argc, Value* args) {
    const char* name = ARG_STR(0);
    static const struct { const char* jp; int code; } map[] = {
        {"上",  ENG_KEY_UP},    {"下", ENG_KEY_DOWN},
        {"左",  ENG_KEY_LEFT},  {"右", ENG_KEY_RIGHT},
        {"決定",ENG_KEY_RETURN},{"スペース",ENG_KEY_SPACE},
        {"エスケープ",ENG_KEY_ESCAPE},
        {"Z",ENG_KEY_Z},{"X",ENG_KEY_X},{"C",ENG_KEY_C},
        {"A",ENG_KEY_A},{"S",ENG_KEY_S},{"D",ENG_KEY_D},{"W",ENG_KEY_W},
        {"シフト",ENG_KEY_LSHIFT},{"コントロール",ENG_KEY_LCTRL},
        {"F1",ENG_KEY_F1},{"F2",ENG_KEY_F2},{"F3",ENG_KEY_F3},
        {"F12",ENG_KEY_F12},
        {NULL, 0}
    };
    for (int i = 0; map[i].jp; ++i)
        if (strcmp(map[i].jp, name) == 0) return NUM(map[i].code);
    return NUM(0);
}

/* ================================================================
 * 描画基本
 * ================================================================ */
static Value fn_描画クリア(int argc, Value* args) {
    eng_clear(g_r, ARG_F(0), ARG_F(1), ARG_F(2), argc > 3 ? ARG_F(3) : 1.0f);
    return NUL;
}
static Value fn_描画フラッシュ(int argc, Value* args) {
    (void)argc; (void)args;
    eng_flush(g_r);
    return NUL;
}

/* ================================================================
 * テクスチャ
 * ================================================================ */
static Value fn_テクスチャ読込(int argc, Value* args)  { return NUM(eng_load_texture(g_r, ARG_STR(0))); }
static Value fn_テクスチャ削除(int argc, Value* args)  { eng_free_texture(g_r, (ENG_TexID)ARG_INT(0)); return NUL; }
static Value fn_テクスチャ幅(int argc, Value* args)    { return NUM(eng_tex_width(g_r, (ENG_TexID)ARG_INT(0))); }
static Value fn_テクスチャ高さ(int argc, Value* args)  { return NUM(eng_tex_height(g_r, (ENG_TexID)ARG_INT(0))); }

/* ================================================================
 * スプライト
 * ================================================================ */
static Value fn_スプライト描画(int argc, Value* args) {
    eng_draw_sprite(g_r, (ENG_TexID)ARG_INT(0),
                    ARG_F(1), ARG_F(2), ARG_F(3), ARG_F(4));
    return NUL;
}
static Value fn_スプライト描画拡張(int argc, Value* args) {
    float rot = argc > 5 ? ARG_F(5) : 0.0f;
    float ox  = argc > 6 ? ARG_F(6) : 0.5f;
    float oy  = argc > 7 ? ARG_F(7) : 0.5f;
    float cr  = argc > 8 ? ARG_F(8) : 1.0f;
    float cg  = argc > 9 ? ARG_F(9) : 1.0f;
    float cb  = argc > 10 ? ARG_F(10) : 1.0f;
    float ca  = argc > 11 ? ARG_F(11) : 1.0f;
    eng_draw_sprite_ex(g_r, (ENG_TexID)ARG_INT(0),
                       ARG_F(1), ARG_F(2), ARG_F(3), ARG_F(4),
                       rot, ox, oy, cr, cg, cb, ca);
    return NUL;
}
static Value fn_スプライト描画UV(int argc, Value* args) {
    eng_draw_sprite_uv(g_r, (ENG_TexID)ARG_INT(0),
                       ARG_F(1), ARG_F(2), ARG_F(3), ARG_F(4),
                       ARG_F(5), ARG_F(6), ARG_F(7), ARG_F(8));
    return NUL;
}
static Value fn_スプライト描画フリップ(int argc, Value* args) {
    bool fx = argc > 5 ? (bool)args[5].boolean : false;
    bool fy = argc > 6 ? (bool)args[6].boolean : false;
    float rot = argc > 7 ? ARG_F(7) : 0.0f;
    float cr = argc > 8  ? ARG_F(8)  : 1.0f;
    float cg = argc > 9  ? ARG_F(9)  : 1.0f;
    float cb = argc > 10 ? ARG_F(10) : 1.0f;
    float ca = argc > 11 ? ARG_F(11) : 1.0f;
    eng_draw_sprite_flip(g_r, (ENG_TexID)ARG_INT(0),
        ARG_F(1), ARG_F(2), ARG_F(3), ARG_F(4),
        fx, fy, rot, cr, cg, cb, ca);
    return NUL;
}

/* ================================================================
 * 図形
 * ================================================================ */
static Value fn_矩形描画(int argc, Value* args) {
    float cr = argc > 4 ? ARG_F(4) : 1.0f;
    float cg = argc > 5 ? ARG_F(5) : 1.0f;
    float cb = argc > 6 ? ARG_F(6) : 1.0f;
    float ca = argc > 7 ? ARG_F(7) : 1.0f;
    eng_draw_rect(g_r, ARG_F(0), ARG_F(1), ARG_F(2), ARG_F(3), cr, cg, cb, ca);
    return NUL;
}
static Value fn_矩形塗潰(int argc, Value* args) {
    float cr = argc > 4 ? ARG_F(4) : 1.0f;
    float cg = argc > 5 ? ARG_F(5) : 1.0f;
    float cb = argc > 6 ? ARG_F(6) : 1.0f;
    float ca = argc > 7 ? ARG_F(7) : 1.0f;
    eng_fill_rect(g_r, ARG_F(0), ARG_F(1), ARG_F(2), ARG_F(3), cr, cg, cb, ca);
    return NUL;
}
static Value fn_円描画(int argc, Value* args) {
    float cr = argc > 3 ? ARG_F(3) : 1.0f;
    float cg = argc > 4 ? ARG_F(4) : 1.0f;
    float cb = argc > 5 ? ARG_F(5) : 1.0f;
    float ca = argc > 6 ? ARG_F(6) : 1.0f;
    eng_draw_circle(g_r, ARG_F(0), ARG_F(1), ARG_F(2), cr, cg, cb, ca);
    return NUL;
}
static Value fn_円塗潰(int argc, Value* args) {
    float cr = argc > 3 ? ARG_F(3) : 1.0f;
    float cg = argc > 4 ? ARG_F(4) : 1.0f;
    float cb = argc > 5 ? ARG_F(5) : 1.0f;
    float ca = argc > 6 ? ARG_F(6) : 1.0f;
    eng_fill_circle(g_r, ARG_F(0), ARG_F(1), ARG_F(2), cr, cg, cb, ca);
    return NUL;
}
static Value fn_直線描画(int argc, Value* args) {
    float cr = argc > 4 ? ARG_F(4) : 1.0f;
    float cg = argc > 5 ? ARG_F(5) : 1.0f;
    float cb = argc > 6 ? ARG_F(6) : 1.0f;
    float ca = argc > 7 ? ARG_F(7) : 1.0f;
    eng_draw_line(g_r, ARG_F(0), ARG_F(1), ARG_F(2), ARG_F(3), cr, cg, cb, ca);
    return NUL;
}

/* ================================================================
 * フォント / テキスト
 * ================================================================ */
static Value fn_フォント読込(int argc, Value* args) {
    return NUM(eng_load_font(g_r, ARG_STR(0), ARG_F(1)));
}
static Value fn_フォント読込デフォルト(int argc, Value* args) {
    return NUM(eng_load_font_default(g_r, argc > 0 ? ARG_F(0) : 24.0f));
}
static Value fn_フォント削除(int argc, Value* args) {
    eng_free_font(g_r, (ENG_FontID)ARG_INT(0));
    return NUL;
}
static Value fn_テキスト描画(int argc, Value* args) {
    float cr = argc > 4 ? ARG_F(4) : 1.0f;
    float cg = argc > 5 ? ARG_F(5) : 1.0f;
    float cb = argc > 6 ? ARG_F(6) : 1.0f;
    float ca = argc > 7 ? ARG_F(7) : 1.0f;
    eng_draw_text(g_r, (ENG_FontID)ARG_INT(0), ARG_STR(1),
                  ARG_F(2), ARG_F(3), cr, cg, cb, ca);
    return NUL;
}
static Value fn_テキスト幅(int argc, Value* args) {
    return NUM(eng_text_width(g_r, (ENG_FontID)ARG_INT(0), ARG_STR(1)));
}
static Value fn_テキスト高さ(int argc, Value* args) {
    return NUM(eng_text_height(g_r, (ENG_FontID)ARG_INT(0)));
}

/* ================================================================
 * ウィンドウ設定 (v1.2.0)
 * ================================================================ */
static Value fn_フルスクリーン設定(int argc, Value* args) { eng_set_fullscreen(g_r, ARG_B(0)); return NUL; }
static Value fn_カーソル表示設定(int argc, Value* args)   { eng_set_cursor_visible(g_r, ARG_B(0)); return NUL; }

/* ================================================================
 * 図形描画 (v1.2.0 追加)
 * ================================================================ */
static Value fn_三角形塗潰(int argc, Value* args) {
    float cr = argc>6?ARG_F(6):1, cg = argc>7?ARG_F(7):1,
          cb = argc>8?ARG_F(8):1, ca = argc>9?ARG_F(9):1;
    eng_fill_tri(g_r, ARG_F(0),ARG_F(1),ARG_F(2),ARG_F(3),ARG_F(4),ARG_F(5), cr,cg,cb,ca);
    return NUL;
}
static Value fn_三角形描画(int argc, Value* args) {
    float cr = argc>6?ARG_F(6):1, cg = argc>7?ARG_F(7):1,
          cb = argc>8?ARG_F(8):1, ca = argc>9?ARG_F(9):1;
    eng_draw_tri(g_r, ARG_F(0),ARG_F(1),ARG_F(2),ARG_F(3),ARG_F(4),ARG_F(5), cr,cg,cb,ca);
    return NUL;
}
static Value fn_カメラ位置設定(int argc, Value* args)  { eng_cam_pos(g_r, ARG_F(0), ARG_F(1)); return NUL; }
static Value fn_カメラズーム設定(int argc, Value* args) { eng_cam_zoom(g_r, ARG_F(0)); return NUL; }
static Value fn_カメラ回転設定(int argc, Value* args)  { eng_cam_rot(g_r, ARG_F(0)); return NUL; }
static Value fn_カメラリセット(int argc, Value* args)  { (void)argc; (void)args; eng_cam_reset(g_r); return NUL; }
static Value fn_カメラX取得(int argc, Value* args)     { (void)argc; (void)args; return NUM(eng_cam_get_x(g_r)); }
static Value fn_カメラY取得(int argc, Value* args)     { (void)argc; (void)args; return NUM(eng_cam_get_y(g_r)); }
static Value fn_カメラズーム取得(int argc, Value* args) { (void)argc; (void)args; return NUM(eng_cam_get_zoom(g_r)); }

/* ================================================================
 * ユーティリティ
 * ================================================================ */
static Value fn_乱数(int argc, Value* args)     { (void)argc; (void)args; return NUM(eng_randf()); }
static Value fn_乱数整数(int argc, Value* args) { return NUM(eng_randi(ARG_INT(0), ARG_INT(1))); }

/* ================================================================
 * プラグイン登録
 * ================================================================ */
#define FN(name, mn, mx) { #name, fn_##name, mn, mx }

static HajimuPluginFunc funcs[] = {
    /* ライフサイクル */
    FN(ウィンドウ作成, 3, 3),
    FN(ウィンドウ削除, 0, 0),
    FN(ウィンドウ更新, 0, 0),
    FN(ウィンドウ幅,   0, 0),
    FN(ウィンドウ高さ, 0, 0),
    FN(経過時間,       0, 0),
    FN(デルタ時間,     0, 0),
    FN(FPS,            0, 0),
    FN(フルスクリーン設定, 1, 1),
    FN(カーソル表示設定,   1, 1),
    /* 入力 */
    FN(キー押下中,        1, 1),
    FN(キー押下,          1, 1),
    FN(キー離した,        1, 1),
    FN(マウスX,           0, 0),
    FN(マウスY,           0, 0),
    FN(マウスボタン押下中, 1, 1),
    FN(マウスボタン押下,   1, 1),
    FN(マウスボタン離した, 1, 1),
    FN(マウスホイール,   0, 0),
    FN(キーコード,         1, 1),
    FN(FPS上限設定,     1, 1),
    /* 描画基本 */
    FN(描画クリア,   0, 4),
    FN(描画フラッシュ, 0, 0),
    /* テクスチャ */
    FN(テクスチャ読込, 1, 1),
    FN(テクスチャ削除, 1, 1),
    FN(テクスチャ幅,   1, 1),
    FN(テクスチャ高さ, 1, 1),
    /* スプライト */
    FN(スプライト描画,     5, 5),
    FN(スプライト描画拡張, 5, 12),
    FN(スプライト描画UV,   9, 9),
    FN(スプライト描画フリップ, 5, 12),
    /* クリッピング */
    FN(クリップ開始, 4, 4),
    FN(クリップ終了, 0, 0),
    /* 図形 */
    FN(矩形描画, 4, 8),
    FN(矩形塗潰, 4, 8),
    FN(円描画,   3, 7),
    FN(円塗潰,   3, 7),
    FN(直線描画, 4, 8),
    FN(三角形描画, 6, 10),
    FN(三角形塗潰, 6, 10),
    /* フォント */
    FN(フォント読込,         2, 2),
    FN(フォント読込デフォルト, 0, 1),
    FN(フォント削除,         1, 1),
    FN(テキスト描画,         4, 8),
    FN(テキスト幅,           2, 2),
    FN(テキスト高さ,         1, 1),
    /* カメラ */
    FN(カメラ位置設定,   2, 2),
    FN(カメラズーム設定, 1, 1),
    FN(カメラ回転設定,   1, 1),
    FN(カメラリセット,   0, 0),
    FN(カメラX取得,      0, 0),
    FN(カメラY取得,      0, 0),
    FN(カメラズーム取得, 0, 0),
    /* ユーティリティ */
    FN(乱数,     0, 0),
    FN(乱数整数, 2, 2),
};

HAJIMU_PLUGIN_EXPORT HajimuPluginInfo* hajimu_plugin_init(void) {
    static HajimuPluginInfo info = {
        .name           = "engine_render",
        .version        = "1.2.0",
        .author         = "Reo Shiozawa",
        .description    = "はじむ用 2D レンダリングエンジン (SDL2 + OpenGL 3.3)",
        .functions      = funcs,
        .function_count = sizeof(funcs) / sizeof(funcs[0]),
    };
    return &info;
}
