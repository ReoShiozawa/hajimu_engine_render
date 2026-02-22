/**
 * src/eng_texture.c — テクスチャ読み込み (stb_image)
 */
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include "eng_internal.h"
#include <stdio.h>
#include <string.h>

/* ── OpenGL テクスチャ生成ヘルパー ──────────────────────*/
static GLuint upload_texture(const unsigned char* pixels,
                             int w, int h, int channels) {
    GLuint tex;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);

    GLenum fmt = (channels == 4) ? GL_RGBA :
                 (channels == 3) ? GL_RGB  :
                 (channels == 2) ? GL_RG   : GL_RED;

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0,
                 fmt, GL_UNSIGNED_BYTE, pixels);

    /* フィルタリング: ピクセルアート向けはNEAREST、通常はLINEAR */
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glBindTexture(GL_TEXTURE_2D, 0);
    return tex;
}

/* ── テクスチャロード ───────────────────────────────────*/
ENG_TexID eng_load_texture(ENG_Renderer* r, const char* path) {
    if (!r || !path) return 0;

    /* 空きスロットを探す */
    int slot = -1;
    for (int i = 0; i < ENG_MAX_TEXTURES; ++i) {
        if (!r->textures[i].used) { slot = i; break; }
    }
    if (slot < 0) {
        fprintf(stderr, "[eng_render] テクスチャスロット不足\n");
        return 0;
    }

    /* stb_image でロード (Y 軸を反転: OpenGL は Y 下→上) */
    stbi_set_flip_vertically_on_load(0); /* スプライトは反転しない */
    int w, h, ch;
    unsigned char* pixels = stbi_load(path, &w, &h, &ch, 4);
    if (!pixels) {
        fprintf(stderr, "[eng_render] テクスチャロード失敗: %s — %s\n",
                path, stbi_failure_reason());
        return 0;
    }

    GLuint gl_id = upload_texture(pixels, w, h, 4);
    stbi_image_free(pixels);

    r->textures[slot].gl_id = gl_id;
    r->textures[slot].w     = w;
    r->textures[slot].h     = h;
    r->textures[slot].used  = true;
    r->tex_count++;

    /* ID は slot + 1 (0 = 無効値 の予約) */
    return (ENG_TexID)(slot + 1);
}

/* ── テクスチャ解放 ─────────────────────────────────────*/
void eng_free_texture(ENG_Renderer* r, ENG_TexID id) {
    if (!r || id == 0) return;
    int slot = (int)id - 1;
    if (slot < 0 || slot >= ENG_MAX_TEXTURES) return;
    if (!r->textures[slot].used) return;

    glDeleteTextures(1, &r->textures[slot].gl_id);
    memset(&r->textures[slot], 0, sizeof(r->textures[slot]));
    r->tex_count--;
}

/* ── クエリ ─────────────────────────────────────────────*/
int eng_tex_width(ENG_Renderer* r, ENG_TexID id) {
    if (!r || id == 0) return 0;
    int slot = (int)id - 1;
    return (slot >= 0 && slot < ENG_MAX_TEXTURES && r->textures[slot].used)
           ? r->textures[slot].w : 0;
}

int eng_tex_height(ENG_Renderer* r, ENG_TexID id) {
    if (!r || id == 0) return 0;
    int slot = (int)id - 1;
    return (slot >= 0 && slot < ENG_MAX_TEXTURES && r->textures[slot].used)
           ? r->textures[slot].h : 0;
}

/* ── テクスチャの GL ID を取得 (バッチ内部用) ──────────*/
GLuint eng_tex_gl_id(ENG_Renderer* r, ENG_TexID id) {
    if (!r || id == 0) return 0;
    int slot = (int)id - 1;
    return (slot >= 0 && slot < ENG_MAX_TEXTURES && r->textures[slot].used)
           ? r->textures[slot].gl_id : 0;
}
