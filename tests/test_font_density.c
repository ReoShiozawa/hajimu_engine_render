/* 同じ文字の論理幅がDPIに依存せず、字形画像だけが高解像度になることを確認する。 */
#include "eng_internal.h"
#include <assert.h>
#include <stdio.h>

int main(void) {
    ENG_Renderer *r = eng_create("Font density regression", 640, 400);
    assert(r);
    r->drawable_w = r->win_w;
    ENG_FontID one = eng_load_font_default(r, 28);
    assert(one);
    float width_one = eng_text_width(r, one, "日本語ABC123");
    r->drawable_w = r->win_w * 2;
    ENG_FontID two = eng_load_font_default(r, 28);
    assert(two);
    float width_two = eng_text_width(r, two, "日本語ABC123");
    assert(fabsf(width_one - width_two) < 0.01f);
    ENG_FontEntry *a = &r->fonts[one - 1], *b = &r->fonts[two - 1];
    assert(a->raster_ratio == 1 && b->raster_ratio == 2);
    assert(b->glyphs[0].h >= a->glyphs[0].h * 2 - 2);
    printf("font density passed: logical width %.2f / %.2f, glyph height %d / %d\n",
           width_one, width_two, a->glyphs[0].h, b->glyphs[0].h);
    eng_destroy(r);
    return 0;
}
