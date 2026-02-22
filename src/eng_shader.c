/**
 * src/eng_shader.c — GLSL シェーダーコンパイル + バッチ初期化
 */
#include "eng_internal.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ── 埋め込みシェーダー ─────────────────────────────────*/
static const char* VERT_SRC =
    "#version 330 core\n"
    "layout(location=0) in vec2 a_pos;\n"
    "layout(location=1) in vec2 a_uv;\n"
    "layout(location=2) in vec4 a_color;\n"
    "out vec2  v_uv;\n"
    "out vec4  v_color;\n"
    "uniform mat4 u_proj;\n"
    "void main() {\n"
    "    gl_Position = u_proj * vec4(a_pos, 0.0, 1.0);\n"
    "    v_uv    = a_uv;\n"
    "    v_color = a_color;\n"
    "}\n";

static const char* FRAG_SRC =
    "#version 330 core\n"
    "in vec2  v_uv;\n"
    "in vec4  v_color;\n"
    "uniform sampler2D u_tex;\n"
    "uniform int u_use_tex;\n"
    "out vec4 frag;\n"
    "void main() {\n"
    "    if (u_use_tex != 0) {\n"
    "        frag = texture(u_tex, v_uv) * v_color;\n"
    "    } else {\n"
    "        frag = v_color;\n"
    "    }\n"
    "}\n";

/* ── シェーダーコンパイルヘルパー ───────────────────────*/
static GLuint compile_shader(GLenum type, const char* src) {
    GLuint sh = glCreateShader(type);
    glShaderSource(sh, 1, &src, NULL);
    glCompileShader(sh);
    GLint ok = 0;
    glGetShaderiv(sh, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[512];
        glGetShaderInfoLog(sh, sizeof(log), NULL, log);
        fprintf(stderr, "[eng_shader] compile error: %s\n", log);
        glDeleteShader(sh);
        return 0;
    }
    return sh;
}

static GLuint link_program(GLuint vert, GLuint frag) {
    GLuint prog = glCreateProgram();
    glAttachShader(prog, vert);
    glAttachShader(prog, frag);
    glLinkProgram(prog);
    GLint ok = 0;
    glGetProgramiv(prog, GL_LINK_STATUS, &ok);
    if (!ok) {
        char log[512];
        glGetProgramInfoLog(prog, sizeof(log), NULL, log);
        fprintf(stderr, "[eng_shader] link error: %s\n", log);
        glDeleteProgram(prog);
        return 0;
    }
    glDeleteShader(vert);
    glDeleteShader(frag);
    return prog;
}

/* ── バッチ用インデックスの事前計算 ─────────────────────*/
static void precompute_indices(uint32_t* indices) {
    /* 各クワッドは 4 頂点 (0,1,2,2,3,0) の三角形 2 枚 */
    for (int i = 0; i < ENG_MAX_BATCH; ++i) {
        uint32_t base = (uint32_t)(i * 4);
        indices[i*6+0] = base+0;
        indices[i*6+1] = base+1;
        indices[i*6+2] = base+2;
        indices[i*6+3] = base+2;
        indices[i*6+4] = base+3;
        indices[i*6+5] = base+0;
    }
}

/* ── シェーダー初期化 ───────────────────────────────────*/
bool eng_shader_init(ENG_Batch* b) {
    GLuint vert = compile_shader(GL_VERTEX_SHADER,   VERT_SRC);
    GLuint frag = compile_shader(GL_FRAGMENT_SHADER, FRAG_SRC);
    if (!vert || !frag) return false;

    b->prog = link_program(vert, frag);
    if (!b->prog) return false;

    b->loc_proj    = glGetUniformLocation(b->prog, "u_proj");
    b->loc_use_tex = glGetUniformLocation(b->prog, "u_use_tex");
    b->loc_tex     = glGetUniformLocation(b->prog, "u_tex");

    return true;
}

/* ── バッチ VAO/VBO/IBO 初期化 ──────────────────────────*/
void eng_batch_init(ENG_Batch* b, GLuint prog) {
    (void)prog;
    b->quad_count  = 0;
    b->current_tex = 0;
    b->use_tex     = false;

    /* VAO */
    glGenVertexArrays(1, &b->vao);
    glBindVertexArray(b->vao);

    /* VBO (動的更新) */
    glGenBuffers(1, &b->vbo);
    glBindBuffer(GL_ARRAY_BUFFER, b->vbo);
    glBufferData(GL_ARRAY_BUFFER,
                 ENG_MAX_BATCH * 4 * sizeof(ENG_Vertex),
                 NULL, GL_DYNAMIC_DRAW);

    /* 属性レイアウト */
    /* location 0: position (xy) */
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE,
        sizeof(ENG_Vertex),
        (void*)offsetof(ENG_Vertex, x));
    glEnableVertexAttribArray(0);
    /* location 1: uv */
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE,
        sizeof(ENG_Vertex),
        (void*)offsetof(ENG_Vertex, u));
    glEnableVertexAttribArray(1);
    /* location 2: color (rgba) */
    glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE,
        sizeof(ENG_Vertex),
        (void*)offsetof(ENG_Vertex, r));
    glEnableVertexAttribArray(2);

    /* IBO (静的: 事前計算済みインデックス) */
    uint32_t* indices = (uint32_t*)malloc(ENG_MAX_BATCH * 6 * sizeof(uint32_t));
    precompute_indices(indices);
    glGenBuffers(1, &b->ibo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, b->ibo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                 ENG_MAX_BATCH * 6 * sizeof(uint32_t),
                 indices, GL_STATIC_DRAW);
    free(indices);

    glBindVertexArray(0);
}

/* ── バッチフラッシュ ───────────────────────────────────*/
void eng_batch_flush(ENG_Renderer* r) {
    ENG_Batch* b = &r->batch;
    if (b->quad_count == 0) return;

    glUseProgram(b->prog);
    glUniform1i(b->loc_use_tex, b->use_tex ? 1 : 0);
    if (b->use_tex) {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, b->current_tex);
        glUniform1i(b->loc_tex, 0);
    }

    glBindVertexArray(b->vao);
    glBindBuffer(GL_ARRAY_BUFFER, b->vbo);
    glBufferSubData(GL_ARRAY_BUFFER, 0,
                    b->quad_count * 4 * sizeof(ENG_Vertex),
                    b->verts);
    glDrawElements(GL_TRIANGLES,
                   b->quad_count * 6,
                   GL_UNSIGNED_INT, NULL);
    b->quad_count  = 0;
    b->current_tex = 0;
}

/* ── クワッド追加 ───────────────────────────────────────*/
void eng_batch_push_quad(ENG_Renderer* r,
    float x, float y, float w, float h,
    float u0, float v0, float u1, float v1,
    float rot, float ox, float oy,
    float cr, float cg, float cb, float ca,
    GLuint tex, bool use_tex)
{
    ENG_Batch* b = &r->batch;

    /* テクスチャ切り替え時またはバッチ満杯時にフラッシュ */
    if (b->quad_count > 0 &&
        (b->current_tex != tex || b->use_tex != use_tex)) {
        eng_batch_flush(r);
    }
    if (b->quad_count >= ENG_MAX_BATCH) {
        eng_batch_flush(r);
    }

    b->current_tex = tex;
    b->use_tex     = use_tex;

    /* 4頂点の座標を計算 (回転あり) */
    float half_w = w * 0.5f;
    float half_h = h * 0.5f;
    /* 原点オフセット (ox,oy は 0.0〜1.0 の割合) */
    float px = x + ox * w;
    float py = y + oy * h;

    float rad = rot * (float)(3.14159265358979323846 / 180.0);
    float cosr = cosf(rad), sinr = sinf(rad);

    /* クワッドの4隅 (回転前の相対座標) */
    float lx = -ox * w, rx = (1.0f - ox) * w;
    float ty = -oy * h, by = (1.0f - oy) * h;
    float corners[4][2] = {
        {lx, ty}, {rx, ty}, {rx, by}, {lx, by}
    };
    float uvs[4][2] = {
        {u0, v0}, {u1, v0}, {u1, v1}, {u0, v1}
    };

    int base = b->quad_count * 4;
    for (int i = 0; i < 4; ++i) {
        float cx2 = corners[i][0];
        float cy2 = corners[i][1];
        b->verts[base+i].x = px + cx2 * cosr - cy2 * sinr;
        b->verts[base+i].y = py + cx2 * sinr + cy2 * cosr;
        b->verts[base+i].u = uvs[i][0];
        b->verts[base+i].v = uvs[i][1];
        b->verts[base+i].r = cr;
        b->verts[base+i].g = cg;
        b->verts[base+i].b = cb;
        b->verts[base+i].a = ca;
    }
    (void)half_w; (void)half_h;
    b->quad_count++;
}
