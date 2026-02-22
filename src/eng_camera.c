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
