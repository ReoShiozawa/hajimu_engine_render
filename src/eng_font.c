/**
 * src/eng_font.c — UTF-8 フォント描画 (stb_truetype)
 *
 * UTF-8をコードポイント単位で処理し、実際に使われた字形だけを
 * 動的アトラスへ追加する。日本語フォント全体を一括ベイクしないため、
 * 起動時間とメモリ使用量を一定に保てる。
 */
#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"

#include "eng_internal.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

static uint32_t eng_utf8_next(const char** cursor) {
    const unsigned char* p = (const unsigned char*)*cursor;
    uint32_t cp;
    int continuation;

    if (p[0] < 0x80) {
        *cursor += 1;
        return p[0];
    }
    if ((p[0] & 0xe0) == 0xc0) {
        cp = p[0] & 0x1f;
        continuation = 1;
        if (cp < 2) goto invalid;
    } else if ((p[0] & 0xf0) == 0xe0) {
        cp = p[0] & 0x0f;
        continuation = 2;
    } else if ((p[0] & 0xf8) == 0xf0) {
        cp = p[0] & 0x07;
        continuation = 3;
    } else {
        goto invalid;
    }

    for (int i = 1; i <= continuation; ++i) {
        if (p[i] == '\0' || (p[i] & 0xc0) != 0x80) goto invalid;
        cp = (cp << 6) | (p[i] & 0x3f);
    }
    if ((continuation == 2 && cp < 0x800) ||
        (continuation == 3 && cp < 0x10000) ||
        (cp >= 0xd800 && cp <= 0xdfff) || cp > 0x10ffff) {
        goto invalid;
    }

    *cursor += continuation + 1;
    return cp;

invalid:
    *cursor += 1;
    return 0xfffd;
}

static ENG_FontGlyph* eng_find_glyph(ENG_FontEntry* fe, uint32_t codepoint) {
    for (int i = 0; i < fe->glyph_count; ++i) {
        if (fe->glyphs[i].codepoint == codepoint) return &fe->glyphs[i];
    }
    return NULL;
}

static ENG_FontGlyph* eng_cache_glyph(ENG_FontEntry* fe, uint32_t codepoint) {
    ENG_FontGlyph* cached = eng_find_glyph(fe, codepoint);
    if (cached) return cached;
    if (fe->glyph_count >= ENG_MAX_FONT_GLYPHS) return NULL;

    stbtt_fontinfo* info = (stbtt_fontinfo*)fe->font_info;
    if (stbtt_FindGlyphIndex(info, (int)codepoint) == 0 && codepoint != '?') {
        codepoint = '?';
        cached = eng_find_glyph(fe, codepoint);
        if (cached) return cached;
    }

    int advance_units = 0;
    int left_bearing = 0;
    int x0 = 0, y0 = 0, x1 = 0, y1 = 0;
    stbtt_GetCodepointHMetrics(info, (int)codepoint, &advance_units, &left_bearing);
    stbtt_GetCodepointBitmapBox(info, (int)codepoint, fe->scale, fe->scale,
                                &x0, &y0, &x1, &y1);

    ENG_FontGlyph* glyph = &fe->glyphs[fe->glyph_count++];
    memset(glyph, 0, sizeof(*glyph));
    glyph->codepoint = codepoint;
    glyph->xoff = (float)x0;
    glyph->yoff = (float)y0;
    glyph->advance = advance_units * fe->scale;

    int w = x1 - x0;
    int h = y1 - y0;
    if (w <= 0 || h <= 0) return glyph;

    const int padding = 1;
    if (fe->atlas_x + w + padding > ENG_FONT_ATLAS_W) {
        fe->atlas_x = padding;
        fe->atlas_y += fe->atlas_row_h + padding;
        fe->atlas_row_h = 0;
    }
    if (fe->atlas_y + h + padding > ENG_FONT_ATLAS_H) {
        fprintf(stderr, "[eng_render] フォントアトラスが満杯です (U+%04X)\n",
                (unsigned int)codepoint);
        fe->glyph_count--;
        return NULL;
    }

    unsigned char* bitmap = stbtt_GetCodepointBitmap(
        info, fe->scale, fe->scale, (int)codepoint, &w, &h, NULL, NULL);
    if (!bitmap) {
        fe->glyph_count--;
        return NULL;
    }

    glyph->x = fe->atlas_x;
    glyph->y = fe->atlas_y;
    glyph->w = w;
    glyph->h = h;
    glBindTexture(GL_TEXTURE_2D, fe->atlas_tex);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexSubImage2D(GL_TEXTURE_2D, 0, glyph->x, glyph->y, w, h,
                    GL_RED, GL_UNSIGNED_BYTE, bitmap);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
    glBindTexture(GL_TEXTURE_2D, 0);
    stbtt_FreeBitmap(bitmap, NULL);

    fe->atlas_x += w + padding;
    if (h > fe->atlas_row_h) fe->atlas_row_h = h;
    return glyph;
}

/* ── フォントロード ─────────────────────────────────────*/
ENG_FontID eng_load_font(ENG_Renderer* r, const char* path, float size) {
    if (!r || !path || size <= 0.0f) return 0;

    int slot = -1;
    for (int i = 0; i < ENG_MAX_FONTS; ++i) {
        if (!r->fonts[i].used) { slot = i; break; }
    }
    if (slot < 0) {
        fprintf(stderr, "[eng_render] フォントスロット不足\n");
        return 0;
    }

    FILE* f = fopen(path, "rb");
    if (!f) {
        fprintf(stderr, "[eng_render] フォント読み込み失敗: %s\n", path);
        return 0;
    }
    fseek(f, 0, SEEK_END);
    long fsz = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (fsz <= 0) {
        fclose(f);
        return 0;
    }
    unsigned char* ttf_buf = (unsigned char*)malloc((size_t)fsz);
    if (!ttf_buf) { fclose(f); return 0; }
    if (fread(ttf_buf, 1, (size_t)fsz, f) < (size_t)fsz) {
        free(ttf_buf);
        fclose(f);
        return 0;
    }
    fclose(f);

    stbtt_fontinfo* info = (stbtt_fontinfo*)malloc(sizeof(stbtt_fontinfo));
    ENG_FontGlyph* glyphs = (ENG_FontGlyph*)calloc(
        ENG_MAX_FONT_GLYPHS, sizeof(ENG_FontGlyph));
    int font_offset = stbtt_GetFontOffsetForIndex(ttf_buf, 0);
    if (!info || !glyphs || font_offset < 0 ||
        !stbtt_InitFont(info, ttf_buf, font_offset)) {
        fprintf(stderr, "[eng_render] フォント解析失敗: %s\n", path);
        free(ttf_buf);
        free(info);
        free(glyphs);
        return 0;
    }

    GLuint tex;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED,
                 ENG_FONT_ATLAS_W, ENG_FONT_ATLAS_H,
                 0, GL_RED, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    GLint swz[4] = {GL_RED, GL_RED, GL_RED, GL_RED};
    glTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_RGBA, swz);
    glBindTexture(GL_TEXTURE_2D, 0);

    ENG_FontEntry* fe = &r->fonts[slot];
    fe->atlas_tex = tex;
    fe->size = size;
    fe->scale = stbtt_ScaleForPixelHeight(info, size);
    fe->ttf_data = ttf_buf;
    fe->font_info = info;
    fe->glyphs = glyphs;
    fe->glyph_count = 0;
    fe->atlas_x = 1;
    fe->atlas_y = 1;
    fe->atlas_row_h = 0;
    fe->used = true;
    r->font_count++;

    return (ENG_FontID)(slot + 1);
}

/* ── システムフォント ───────────────────────────────────*/
ENG_FontID eng_load_font_default(ENG_Renderer* r, float size) {
    const char* candidates[] = {
        "/System/Library/Fonts/Supplemental/AppleGothic.ttf",
        "/System/Library/Fonts/AppleSDGothicNeo.ttc",
        "/System/Library/Fonts/Supplemental/Arial.ttf",
        "/Library/Fonts/Arial.ttf",
        "/usr/share/fonts/opentype/noto/NotoSansCJK-Regular.ttc",
        "/usr/share/fonts/opentype/noto/NotoSansCJKjp-Regular.otf",
        "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
        "/usr/share/fonts/TTF/DejaVuSans.ttf",
        "C:\\Windows\\Fonts\\YuGothM.ttc",
        "C:\\Windows\\Fonts\\meiryo.ttc",
        "C:\\Windows\\Fonts\\arial.ttf",
        NULL
    };
    for (int i = 0; candidates[i]; ++i) {
        FILE* f = fopen(candidates[i], "rb");
        if (f) {
            fclose(f);
            ENG_FontID id = eng_load_font(r, candidates[i], size);
            if (id != 0) return id;
        }
    }
    fprintf(stderr, "[eng_render] デフォルトフォントが見つかりません\n");
    return 0;
}

/* ── フォント解放 ───────────────────────────────────────*/
void eng_free_font(ENG_Renderer* r, ENG_FontID id) {
    if (!r || id == 0) return;
    int slot = (int)id - 1;
    if (slot < 0 || slot >= ENG_MAX_FONTS) return;
    ENG_FontEntry* fe = &r->fonts[slot];
    if (!fe->used) return;
    glDeleteTextures(1, &fe->atlas_tex);
    free(fe->glyphs);
    free(fe->font_info);
    free(fe->ttf_data);
    memset(fe, 0, sizeof(*fe));
    r->font_count--;
}

/* ── 字形描画 ───────────────────────────────────────────*/
static void eng_draw_glyph(ENG_Renderer* r, ENG_FontEntry* fe,
                           ENG_FontGlyph* glyph, float x, float y,
                           float cr, float cg, float cb, float ca) {
    if (!glyph || glyph->w <= 0 || glyph->h <= 0) return;
    float aw = (float)ENG_FONT_ATLAS_W;
    float ah = (float)ENG_FONT_ATLAS_H;
    eng_batch_push_quad(r,
        x + glyph->xoff, y + glyph->yoff,
        (float)glyph->w, (float)glyph->h,
        glyph->x / aw, glyph->y / ah,
        (glyph->x + glyph->w) / aw,
        (glyph->y + glyph->h) / ah,
        0.0f, 0.0f, 0.0f,
        cr, cg, cb, ca,
        fe->atlas_tex, true
    );
}

/* ── テキスト描画 ───────────────────────────────────────*/
void eng_draw_text(ENG_Renderer* r, ENG_FontID fid,
                   const char* text,
                   float x, float y,
                   float cr, float cg, float cb, float ca) {
    if (!r || fid == 0 || !text) return;
    int slot = (int)fid - 1;
    if (slot < 0 || slot >= ENG_MAX_FONTS || !r->fonts[slot].used) return;

    ENG_FontEntry* fe = &r->fonts[slot];
    float cx = x;
    float cy = y;
    const char* p = text;
    while (*p) {
        uint32_t codepoint = eng_utf8_next(&p);
        if (codepoint == '\n') {
            cx = x;
            cy += fe->size * 1.2f;
            continue;
        }
        ENG_FontGlyph* glyph = eng_cache_glyph(fe, codepoint);
        if (!glyph) continue;
        eng_draw_glyph(r, fe, glyph, cx, cy, cr, cg, cb, ca);
        cx += glyph->advance;
    }
}

/* ── テキスト幅計算 ─────────────────────────────────────*/
float eng_text_width(ENG_Renderer* r, ENG_FontID fid, const char* text) {
    if (!r || fid == 0 || !text) return 0.0f;
    int slot = (int)fid - 1;
    if (slot < 0 || slot >= ENG_MAX_FONTS || !r->fonts[slot].used) return 0.0f;

    ENG_FontEntry* fe = &r->fonts[slot];
    float width = 0.0f;
    float max_width = 0.0f;
    const char* p = text;
    while (*p) {
        uint32_t codepoint = eng_utf8_next(&p);
        if (codepoint == '\n') {
            if (width > max_width) max_width = width;
            width = 0.0f;
            continue;
        }
        ENG_FontGlyph* glyph = eng_cache_glyph(fe, codepoint);
        if (glyph) width += glyph->advance;
    }
    return width > max_width ? width : max_width;
}

/* ── テキスト行高さ ─────────────────────────────────────*/
float eng_text_height(ENG_Renderer* r, ENG_FontID fid) {
    if (!r || fid == 0) return 0.0f;
    int slot = (int)fid - 1;
    if (slot < 0 || slot >= ENG_MAX_FONTS || !r->fonts[slot].used) return 0.0f;
    return r->fonts[slot].size;
}

/* ── テキスト折り返し描画 ───────────────────────────────*/
void eng_draw_text_wrap(ENG_Renderer* r, ENG_FontID fid,
                        const char* text,
                        float x, float y, float max_w,
                        float cr, float cg, float cb, float ca) {
    if (!r || !text || fid == 0 || max_w <= 0.0f) return;
    int slot = (int)fid - 1;
    if (slot < 0 || slot >= ENG_MAX_FONTS || !r->fonts[slot].used) return;

    ENG_FontEntry* fe = &r->fonts[slot];
    float line_h = fe->size * 1.2f;
    float cx = x;
    float cy = y;
    const char* p = text;
    while (*p) {
        uint32_t codepoint = eng_utf8_next(&p);
        if (codepoint == '\n') {
            cx = x;
            cy += line_h;
            continue;
        }

        ENG_FontGlyph* glyph = eng_cache_glyph(fe, codepoint);
        if (!glyph) continue;
        if (cx + glyph->advance > x + max_w && cx > x) {
            cx = x;
            cy += line_h;
        }
        eng_draw_glyph(r, fe, glyph, cx, cy, cr, cg, cb, ca);
        cx += glyph->advance;
    }
}
