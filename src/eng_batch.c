/**
 * src/eng_batch.c — スプライト・図形描画
 */
#include "eng_internal.h"
#include <math.h>
#include <string.h>

/* ── 描画クリア ─────────────────────────────────────────*/
void eng_clear(ENG_Renderer* r, float red, float green, float blue, float alpha) {
    if (!r) return;
    eng_batch_flush(r);  /* 残りバッチをフラッシュ */
    glClearColor(red, green, blue, alpha);
    glClear(GL_COLOR_BUFFER_BIT);
}

/* ── フラッシュ (フレーム末尾) ──────────────────────────*/
void eng_flush(ENG_Renderer* r) {
    if (!r) return;
    eng_batch_flush(r);
    SDL_GL_SwapWindow(r->window);
}

/* ── スプライト描画 ─────────────────────────────────────*/
void eng_draw_sprite(ENG_Renderer* r, ENG_TexID id,
                     float x, float y, float w, float h) {
    if (!r || !id) return;
    GLuint gl_id = eng_tex_gl_id(r, id);
    if (!gl_id) return;
    eng_batch_push_quad(r,
        x, y, w, h,
        0.0f, 0.0f, 1.0f, 1.0f,
        0.0f, 0.0f, 0.0f,
        1.0f, 1.0f, 1.0f, 1.0f,
        gl_id, true
    );
}

void eng_draw_sprite_ex(ENG_Renderer* r, ENG_TexID id,
                        float x, float y, float w, float h,
                        float rot, float ox, float oy,
                        float cr, float cg, float cb, float ca) {
    if (!r || !id) return;
    GLuint gl_id = eng_tex_gl_id(r, id);
    if (!gl_id) return;
    eng_batch_push_quad(r,
        x, y, w, h,
        0.0f, 0.0f, 1.0f, 1.0f,
        rot, ox, oy,
        cr, cg, cb, ca,
        gl_id, true
    );
}

void eng_draw_sprite_uv(ENG_Renderer* r, ENG_TexID id,
                        float x,  float y,  float w,  float h,
                        float u0, float v0, float u1, float v1) {
    if (!r || !id) return;
    GLuint gl_id = eng_tex_gl_id(r, id);
    if (!gl_id) return;
    eng_batch_push_quad(r,
        x, y, w, h,
        u0, v0, u1, v1,
        0.0f, 0.0f, 0.0f,
        1.0f, 1.0f, 1.0f, 1.0f,
        gl_id, true
    );
}

void eng_draw_sprite_uv_ex(ENG_Renderer* r, ENG_TexID id,
                           float x,  float y,  float w,  float h,
                           float u0, float v0, float u1, float v1,
                           float rot, float ox, float oy,
                           float cr, float cg, float cb, float ca) {
    if (!r || !id) return;
    GLuint gl_id = eng_tex_gl_id(r, id);
    if (!gl_id) return;
    eng_batch_push_quad(r,
        x, y, w, h,
        u0, v0, u1, v1,
        rot, ox, oy,
        cr, cg, cb, ca,
        gl_id, true
    );
}

/* ── 矩形 ───────────────────────────────────────────────*/
void eng_fill_rect(ENG_Renderer* r,
                   float x, float y, float w, float h,
                   float cr, float cg, float cb, float ca) {
    if (!r) return;
    eng_batch_push_quad(r,
        x, y, w, h,
        0.0f, 0.0f, 1.0f, 1.0f,
        0.0f, 0.0f, 0.0f,
        cr, cg, cb, ca,
        0, false
    );
}

void eng_draw_rect(ENG_Renderer* r,
                   float x, float y, float w, float h,
                   float cr, float cg, float cb, float ca) {
    if (!r) return;
    float t = 1.0f; /* 線幅 1px */
    eng_fill_rect(r, x,       y,       w, t, cr, cg, cb, ca);  /* 上 */
    eng_fill_rect(r, x,       y+h-t,   w, t, cr, cg, cb, ca);  /* 下 */
    eng_fill_rect(r, x,       y,       t, h, cr, cg, cb, ca);  /* 左 */
    eng_fill_rect(r, x+w-t,   y,       t, h, cr, cg, cb, ca);  /* 右 */
}

/* ── 円 ─────────────────────────────────────────────────*/
/* 円は小さな矩形のセグメントで近似する簡易実装 */
void eng_fill_circle(ENG_Renderer* r,
                     float cx, float cy, float radius,
                     float cr, float cg, float cb, float ca) {
    if (!r) return;
    int segs = 32;
    float step = (float)(2.0 * 3.14159265358979323846 / segs);
    for (int i = 0; i < segs; ++i) {
        float a0 = i * step;
        float a1 = a0 + step;
        float x0 = cx + cosf(a0) * radius;
        float y0 = cy + sinf(a0) * radius;
        float x1 = cx;
        float y1 = cy;
        float x2 = cx + cosf(a1) * radius;
        float y2 = cy + sinf(a1) * radius;
        /* 三角形を 縦幅0の四角形で近似（eng_fill_rectで代替） */
        /* 頂点を直接 push */
        ENG_Batch* b = &r->batch;
        if (b->quad_count > 0 && b->use_tex) eng_batch_flush(r);
        if (b->quad_count >= ENG_MAX_BATCH) eng_batch_flush(r);
        b->use_tex = false; b->current_tex = 0;
        int base = b->quad_count * 4;
        /* 縮退した4頂点 (p0, p1, p2, p2) */
        b->verts[base+0] = (ENG_Vertex){x0,y0, 0,0, cr,cg,cb,ca};
        b->verts[base+1] = (ENG_Vertex){x1,y1, 0,0, cr,cg,cb,ca};
        b->verts[base+2] = (ENG_Vertex){x2,y2, 0,0, cr,cg,cb,ca};
        b->verts[base+3] = (ENG_Vertex){x2,y2, 0,0, cr,cg,cb,ca};
        b->quad_count++;
    }
}

void eng_draw_circle(ENG_Renderer* r,
                     float cx, float cy, float radius,
                     float cr, float cg, float cb, float ca) {
    if (!r) return;
    int segs = 32;
    float step = (float)(2.0 * 3.14159265358979323846 / segs);
    for (int i = 0; i < segs; ++i) {
        float a0 = i * step;
        float a1 = a0 + step;
        float x0 = cx + cosf(a0) * radius;
        float y0 = cy + sinf(a0) * radius;
        float x1 = cx + cosf(a1) * radius;
        float y1 = cy + sinf(a1) * radius;
        eng_draw_line(r, x0, y0, x1, y1, cr, cg, cb, ca);
    }
}

/* ── 直線 ───────────────────────────────────────────────*/
void eng_draw_line(ENG_Renderer* r,
                   float x1, float y1, float x2, float y2,
                   float cr, float cg, float cb, float ca) {
    if (!r) return;
    float dx = x2 - x1, dy = y2 - y1;
    float len = sqrtf(dx*dx + dy*dy);
    if (len < 0.001f) return;
    float nx = -dy / len, ny = dx / len;
    float t = 1.0f; /* 線幅 1px */
    float hx = nx * t * 0.5f, hy = ny * t * 0.5f;
    /* 直線を薄い矩形に変換 */
    ENG_Batch* b = &r->batch;
    if (b->quad_count > 0 && b->use_tex) eng_batch_flush(r);
    if (b->quad_count >= ENG_MAX_BATCH)  eng_batch_flush(r);
    b->use_tex = false; b->current_tex = 0;
    int base = b->quad_count * 4;
    b->verts[base+0] = (ENG_Vertex){x1+hx, y1+hy, 0,0, cr,cg,cb,ca};
    b->verts[base+1] = (ENG_Vertex){x2+hx, y2+hy, 0,0, cr,cg,cb,ca};
    b->verts[base+2] = (ENG_Vertex){x2-hx, y2-hy, 0,0, cr,cg,cb,ca};
    b->verts[base+3] = (ENG_Vertex){x1-hx, y1-hy, 0,0, cr,cg,cb,ca};
    b->quad_count++;
}

/* ── フリップ描画 ───────────────────────────────────────*/
void eng_draw_sprite_flip(ENG_Renderer* r, ENG_TexID id,
                          float x, float y, float w, float h,
                          bool flip_x, bool flip_y,
                          float rot,
                          float cr, float cg, float cb, float ca) {
    if (!r || !id) return;
    GLuint gl_id = eng_tex_gl_id(r, id);
    if (!gl_id) return;
    float u0 = flip_x ? 1.0f : 0.0f;
    float u1 = flip_x ? 0.0f : 1.0f;
    float v0 = flip_y ? 1.0f : 0.0f;
    float v1 = flip_y ? 0.0f : 1.0f;
    eng_batch_push_quad(r,
        x, y, w, h,
        u0, v0, u1, v1,
        rot, 0.5f, 0.5f,
        cr, cg, cb, ca,
        gl_id, true
    );
}

/* ── クリッピング ───────────────────────────────────────*/
void eng_clip_begin(ENG_Renderer* r, float x, float y, float w, float h) {
    if (!r) return;
    eng_batch_flush(r);  /* 既存描画を先にフラッシュ */
    r->clip_active = true;
    r->clip_x = (int)x;
    r->clip_y = r->win_h - (int)(y + h);  /* OpenGL は下基準 */
    r->clip_w = (int)w;
    r->clip_h = (int)h;
    glEnable(GL_SCISSOR_TEST);
    glScissor(r->clip_x, r->clip_y, r->clip_w, r->clip_h);
}

void eng_clip_end(ENG_Renderer* r) {
    if (!r) return;
    eng_batch_flush(r);
    r->clip_active = false;
    glDisable(GL_SCISSOR_TEST);
}
