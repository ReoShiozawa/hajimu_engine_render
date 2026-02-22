/**
 * src/eng_camera.c — 2D カメラ制御
 */
#include "eng_internal.h"

void eng_cam_pos(ENG_Renderer* r, float x, float y) {
    if (!r) return;
    r->cam_x = x; r->cam_y = y;
    eng_batch_flush(r);  /* 射影変更前にフラッシュ */
    eng_update_proj(r);
}

void eng_cam_zoom(ENG_Renderer* r, float zoom) {
    if (!r || zoom <= 0.001f) return;
    r->cam_zoom = zoom;
    eng_batch_flush(r);
    eng_update_proj(r);
}

void eng_cam_rot(ENG_Renderer* r, float degrees) {
    if (!r) return;
    r->cam_rot_deg = degrees;
    eng_batch_flush(r);
    eng_update_proj(r);
}

void eng_cam_reset(ENG_Renderer* r) {
    if (!r) return;
    r->cam_x = 0.0f; r->cam_y = 0.0f;
    r->cam_zoom = 1.0f; r->cam_rot_deg = 0.0f;
    eng_batch_flush(r);
    eng_update_proj(r);
}

float eng_cam_get_x(ENG_Renderer* r)    { return r ? r->cam_x       : 0.0f; }
float eng_cam_get_y(ENG_Renderer* r)    { return r ? r->cam_y       : 0.0f; }
float eng_cam_get_zoom(ENG_Renderer* r) { return r ? r->cam_zoom    : 1.0f; }

/* ── 座標変換 ───────────────────────────────────────────*/
/* 変換式の導出:
 *   world→NDC: NDC_x = (2z/W) * (ca*(wx-cx) + sa*(wy-cy)) - 1
 *   逆変換 (screen→world, rotation=a, zoom=z, cam=(cx,cy)):
 *     wx = cx + ca*(sx/z) - sa*(sy/z)
 *     wy = cy + sa*(sx/z) + ca*(sy/z)
 *   ただし sx = screen_x, sy = screen_y (左上原点ピクセル)
 */
void eng_cam_screen_to_world(ENG_Renderer* r,
                              float sx, float sy,
                              float* wx, float* wy) {
    if (!r || !wx || !wy) return;
    float a  = r->cam_rot_deg * (float)(3.14159265358979323846 / 180.0);
    float ca = cosf(a), sa = sinf(a);
    float z  = r->cam_zoom > 0.001f ? r->cam_zoom : 0.001f;
    float px = sx / z;
    float py = sy / z;
    *wx = r->cam_x + ca * px - sa * py;
    *wy = r->cam_y + sa * px + ca * py;
}

void eng_cam_world_to_screen(ENG_Renderer* r,
                              float wx, float wy,
                              float* sx, float* sy) {
    if (!r || !sx || !sy) return;
    float a  = r->cam_rot_deg * (float)(3.14159265358979323846 / 180.0);
    float ca = cosf(a), sa = sinf(a);
    float dx = wx - r->cam_x;
    float dy = wy - r->cam_y;
    *sx = (ca * dx + sa * dy) * r->cam_zoom;
    *sy = (-sa * dx + ca * dy) * r->cam_zoom;
}
