/**
 * src/eng_font.c — フォント描画 (stb_truetype)
 *
 * ASCII + 基本ラテン / ラテン補完 文字をサポート。
 * 日本語等は将来バージョンで対応予定。
 */
#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"

#include "eng_internal.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* ── フォントロード ─────────────────────────────────────*/
ENG_FontID eng_load_font(ENG_Renderer* r, const char* path, float size) {
    if (!r || !path) return 0;

    /* 空きスロット */
    int slot = -1;
    for (int i = 0; i < ENG_MAX_FONTS; ++i) {
        if (!r->fonts[i].used) { slot = i; break; }
    }
    if (slot < 0) {
        fprintf(stderr, "[eng_render] フォントスロット不足\n");
        return 0;
    }

    /* TTF ファイル読み込み */
    FILE* f = fopen(path, "rb");
    if (!f) {
        fprintf(stderr, "[eng_render] フォント読み込み失敗: %s\n", path);
        return 0;
    }
    fseek(f, 0, SEEK_END);
    long fsz = ftell(f);
    fseek(f, 0, SEEK_SET);
    unsigned char* ttf_buf = (unsigned char*)malloc((size_t)fsz);
    if (!ttf_buf) { fclose(f); return 0; }
    if (fread(ttf_buf, 1, (size_t)fsz, f) < (size_t)fsz) {
        free(ttf_buf); fclose(f); return 0;
    }
    fclose(f);

    /* アトラスベイク (ASCII 32〜127) */
    unsigned char* atlas_pix = (unsigned char*)calloc(
        ENG_FONT_ATLAS_W * ENG_FONT_ATLAS_H, 1);
    stbtt_bakedchar* baked = (stbtt_bakedchar*)malloc(96 * sizeof(stbtt_bakedchar));
    if (!atlas_pix || !baked) {
        free(ttf_buf); free(atlas_pix); free(baked);
        return 0;
    }

    int ret = stbtt_BakeFontBitmap(
        ttf_buf, 0, size,
        atlas_pix, ENG_FONT_ATLAS_W, ENG_FONT_ATLAS_H,
        32, 96, baked);
    free(ttf_buf);

    if (ret <= 0) {
        fprintf(stderr, "[eng_render] フォントベイク失敗 (解像度を下げてください)\n");
        free(atlas_pix); free(baked);
        return 0;
    }

    /* GL テクスチャに転送 (1チャネル) */
    GLuint tex;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED,
                 ENG_FONT_ATLAS_W, ENG_FONT_ATLAS_H,
                 0, GL_RED, GL_UNSIGNED_BYTE, atlas_pix);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    /* swizzle: R→RGBA (白テキスト) */
    GLint swz[4] = {GL_RED, GL_RED, GL_RED, GL_RED};
    glTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_RGBA, swz);
    glBindTexture(GL_TEXTURE_2D, 0);
    free(atlas_pix);

    r->fonts[slot].atlas_tex = tex;
    r->fonts[slot].size      = size;
    r->fonts[slot].baked     = baked;
    r->fonts[slot].used      = true;
    r->font_count++;

    return (ENG_FontID)(slot + 1);
}

/* ── システムフォント ───────────────────────────────────*/
ENG_FontID eng_load_font_default(ENG_Renderer* r, float size) {
    /* macOS */
    const char* candidates[] = {
        "/System/Library/Fonts/Supplemental/Arial.ttf",
        "/Library/Fonts/Arial.ttf",
        "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
        "/usr/share/fonts/TTF/DejaVuSans.ttf",
        "C:\\Windows\\Fonts\\arial.ttf",
        NULL
    };
    for (int i = 0; candidates[i]; ++i) {
        FILE* f = fopen(candidates[i], "rb");
        if (f) { fclose(f); return eng_load_font(r, candidates[i], size); }
    }
    fprintf(stderr, "[eng_render] デフォルトフォントが見つかりません\n");
    return 0;
}

/* ── フォント解放 ───────────────────────────────────────*/
void eng_free_font(ENG_Renderer* r, ENG_FontID id) {
    if (!r || id == 0) return;
    int slot = (int)id - 1;
    if (slot < 0 || slot >= ENG_MAX_FONTS) return;
    if (!r->fonts[slot].used) return;
    glDeleteTextures(1, &r->fonts[slot].atlas_tex);
    free(r->fonts[slot].baked);
    memset(&r->fonts[slot], 0, sizeof(r->fonts[slot]));
    r->font_count--;
}

/* ── テキスト描画 ───────────────────────────────────────*/
void eng_draw_text(ENG_Renderer* r, ENG_FontID fid,
                   const char* text,
                   float x, float y,
                   float cr, float cg, float cb, float ca) {
    if (!r || fid == 0 || !text) return;
    int slot = (int)fid - 1;
    if (slot < 0 || slot >= ENG_MAX_FONTS || !r->fonts[slot].used) return;

    ENG_FontEntry*   fe    = &r->fonts[slot];
    stbtt_bakedchar* baked = (stbtt_bakedchar*)fe->baked;
    GLuint           gl_id = fe->atlas_tex;
    float aw = (float)ENG_FONT_ATLAS_W;
    float ah = (float)ENG_FONT_ATLAS_H;

    float cx2 = x, cy2 = y;
    for (const char* p = text; *p; ++p) {
        unsigned char c = (unsigned char)*p;
        if (c < 32 || c >= 128) {
            cx2 += fe->size * 0.5f;
            continue;
        }
        stbtt_bakedchar* b = &baked[c - 32];
        float qx = cx2 + b->xoff;
        float qy = cy2 + b->yoff;
        float qw = (float)(b->x1 - b->x0);
        float qh = (float)(b->y1 - b->y0);
        float u0 = b->x0 / aw, v0 = b->y0 / ah;
        float u1 = b->x1 / aw, v1 = b->y1 / ah;
        eng_batch_push_quad(r,
            qx, qy, qw, qh,
            u0, v0, u1, v1,
            0.0f, 0.0f, 0.0f,
            cr, cg, cb, ca,
            gl_id, true
        );
        cx2 += b->xadvance;
    }
}

/* ── テキスト幅計算 ─────────────────────────────────────*/
float eng_text_width(ENG_Renderer* r, ENG_FontID fid, const char* text) {
    if (!r || fid == 0 || !text) return 0.0f;
    int slot = (int)fid - 1;
    if (slot < 0 || slot >= ENG_MAX_FONTS || !r->fonts[slot].used) return 0.0f;
    stbtt_bakedchar* baked = (stbtt_bakedchar*)r->fonts[slot].baked;
    float w = 0.0f;
    for (const char* p = text; *p; ++p) {
        unsigned char c = (unsigned char)*p;
        if (c < 32 || c >= 128) { w += r->fonts[slot].size * 0.5f; continue; }
        w += baked[c - 32].xadvance;
    }
    return w;
}
