#include "retro_sky.h"

#include <math.h>
#include <string.h>

#include "retro_material.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

static float clamp01f(float v) {
    if (v < 0.0f) return 0.0f;
    if (v > 1.0f) return 1.0f;
    return v;
}

static float smoothstepf(float edge0, float edge1, float x) {
    float t = clamp01f((x - edge0) / (edge1 - edge0));
    return t * t * (3.0f - 2.0f * t);
}

static float fracf(float v) {
    return v - floorf(v);
}

void retro_eval_sun_dir(float time_sec, float *out_x, float *out_y, float *out_z) {
    float orbit_t = time_sec * 0.025f;
    float tilt = 0.40f;
    float sx = cosf(orbit_t);
    float sy = sinf(orbit_t) * cosf(tilt);
    float sz = sinf(orbit_t) * sinf(tilt);

    if (out_x) *out_x = sx;
    if (out_y) *out_y = sy;
    if (out_z) *out_z = sz;
}

void retro_eval_sky_fog_rgb(float time_sec, float *out_r, float *out_g, float *out_b) {
    float sx = 0.0f, sy = 0.0f, sz = 0.0f;
    retro_eval_sun_dir(time_sec, &sx, &sy, &sz);
    float day = smoothstepf(-0.26f, 0.18f, sy);
    float r = lerpf(0.17f, 0.63f, day);
    float g = lerpf(0.23f, 0.77f, day);
    float b = lerpf(0.33f, 0.96f, day);
    if (out_r) *out_r = r;
    if (out_g) *out_g = g;
    if (out_b) *out_b = b;
}

static void fill_cloud_texture(ProcTexture *t) {
    if (!t || !t->pixels) return;
    for (int y = 0; y < t->height; ++y) {
        for (int x = 0; x < t->width; ++x) {
            float u = (float)x / (float)t->width;
            float v = (float)y / (float)t->height;
            float n1 = 0.5f + 0.5f * sinf((u * 5.2f + v * 2.1f) * (float)M_PI * 2.0f);
            float n2 = 0.5f + 0.5f * sinf((u * 10.8f - v * 3.7f + 0.21f) * (float)M_PI * 2.0f);
            float n3 = 0.5f + 0.5f * sinf((u * 21.0f + v * 14.0f + 1.7f) * (float)M_PI * 2.0f);
            float n = n1 * 0.52f + n2 * 0.33f + n3 * 0.15f;
            float wisps = 0.5f + 0.5f * sinf((u - v) * 12.0f + n * 3.0f);
            float body = smoothstepf(0.56f, 0.80f, n * 0.85f + wisps * 0.15f);

            size_t idx = ((size_t)y * (size_t)t->width + (size_t)x) * 4u;
            unsigned char rgb = (unsigned char)(220.0f + body * 28.0f);
            unsigned char alpha = (unsigned char)(body * 150.0f);
            t->pixels[idx + 0] = rgb;
            t->pixels[idx + 1] = rgb;
            t->pixels[idx + 2] = (unsigned char)(rgb + 4);
            t->pixels[idx + 3] = alpha;
        }
    }
}

static void fill_disc_texture(ProcTexture *t, float r, float g, float b, float edge_softness) {
    if (!t || !t->pixels) return;
    for (int y = 0; y < t->height; ++y) {
        for (int x = 0; x < t->width; ++x) {
            float u = ((float)x + 0.5f) / (float)t->width;
            float v = ((float)y + 0.5f) / (float)t->height;
            float dx = u - 0.5f;
            float dy = v - 0.5f;
            float d = sqrtf(dx * dx + dy * dy);
            float glow = smoothstepf(0.52f, 0.0f, d);
            float core = smoothstepf(0.40f + edge_softness, 0.0f, d);
            float alpha = clamp01f(glow * 0.72f + core * 0.48f);

            size_t idx = ((size_t)y * (size_t)t->width + (size_t)x) * 4u;
            t->pixels[idx + 0] = (unsigned char)(clamp01f(r * (0.75f + core * 0.25f)) * 255.0f);
            t->pixels[idx + 1] = (unsigned char)(clamp01f(g * (0.75f + core * 0.25f)) * 255.0f);
            t->pixels[idx + 2] = (unsigned char)(clamp01f(b * (0.75f + core * 0.25f)) * 255.0f);
            t->pixels[idx + 3] = (unsigned char)(alpha * 255.0f);
        }
    }
}

static void draw_sky_cube(float s) {
    const float top_r = 0.60f, top_g = 0.76f, top_b = 0.94f;
    const float mid_r = 0.56f, mid_g = 0.73f, mid_b = 0.92f;
    const float low_r = 0.65f, low_g = 0.78f, low_b = 0.93f;

    glBegin(GL_QUADS);
    glColor3f(top_r, top_g, top_b);
    glVertex3f(-s, s, -s); glVertex3f(-s, s, s); glVertex3f(s, s, s); glVertex3f(s, s, -s);

    glColor3f(mid_r, mid_g, mid_b);
    glVertex3f(-s, -s, -s); glVertex3f(-s, s, -s); glVertex3f(s, s, -s); glVertex3f(s, -s, -s);
    glVertex3f(s, -s, s); glVertex3f(s, s, s); glVertex3f(-s, s, s); glVertex3f(-s, -s, s);
    glVertex3f(-s, -s, s); glVertex3f(-s, s, s); glVertex3f(-s, s, -s); glVertex3f(-s, -s, -s);
    glVertex3f(s, -s, -s); glVertex3f(s, s, -s); glVertex3f(s, s, s); glVertex3f(s, -s, s);

    glColor3f(low_r, low_g, low_b);
    glVertex3f(-s, -s, -s); glVertex3f(s, -s, -s); glVertex3f(s, -s, s); glVertex3f(-s, -s, s);
    glEnd();
}

static void draw_cloud_layer(GLuint cloud_tex, float size, float y, float uv_scroll, float alpha) {
    RetroMaterial mat = {
        cloud_tex,
        {1.0f, 1.0f, 1.0f, alpha},
        1,
        GL_SRC_ALPHA,
        GL_ONE_MINUS_SRC_ALPHA
    };
    RetroMaterialState prev;
    retro_material_apply(&mat, &prev);

    const float t = uv_scroll;
    glBegin(GL_QUADS);
    glTexCoord2f(0.0f + t, 0.0f + t * 0.35f); glVertex3f(-size, y, -size);
    glTexCoord2f(2.5f + t, 0.0f + t * 0.35f); glVertex3f(size, y, -size);
    glTexCoord2f(2.5f + t, 2.5f + t * 0.35f); glVertex3f(size, y, size);
    glTexCoord2f(0.0f + t, 2.5f + t * 0.35f); glVertex3f(-size, y, size);
    glEnd();

    retro_material_restore(&prev);
}

static void draw_orb_billboard(GLuint tex_id, float px, float py, float pz, float radius, float alpha) {
    float dir_x = px;
    float dir_y = py;
    float dir_z = pz;
    float len = sqrtf(dir_x * dir_x + dir_y * dir_y + dir_z * dir_z);
    if (len < 0.0001f) return;
    dir_x /= len;
    dir_y /= len;
    dir_z /= len;

    float up_x = 0.0f, up_y = 1.0f, up_z = 0.0f;
    float right_x = up_y * dir_z - up_z * dir_y;
    float right_y = up_z * dir_x - up_x * dir_z;
    float right_z = up_x * dir_y - up_y * dir_x;
    float right_len = sqrtf(right_x * right_x + right_y * right_y + right_z * right_z);
    if (right_len < 0.0001f) {
        up_x = 0.0f; up_y = 0.0f; up_z = 1.0f;
        right_x = up_y * dir_z - up_z * dir_y;
        right_y = up_z * dir_x - up_x * dir_z;
        right_z = up_x * dir_y - up_y * dir_x;
        right_len = sqrtf(right_x * right_x + right_y * right_y + right_z * right_z);
        if (right_len < 0.0001f) return;
    }
    right_x /= right_len;
    right_y /= right_len;
    right_z /= right_len;

    float real_up_x = dir_y * right_z - dir_z * right_y;
    float real_up_y = dir_z * right_x - dir_x * right_z;
    float real_up_z = dir_x * right_y - dir_y * right_x;

    RetroMaterial mat = {
        tex_id,
        {1.0f, 1.0f, 1.0f, alpha},
        1,
        GL_SRC_ALPHA,
        GL_ONE_MINUS_SRC_ALPHA
    };
    RetroMaterialState prev;
    retro_material_apply(&mat, &prev);

    glBegin(GL_QUADS);
    glTexCoord2f(0.0f, 0.0f); glVertex3f(px - right_x * radius - real_up_x * radius, py - right_y * radius - real_up_y * radius, pz - right_z * radius - real_up_z * radius);
    glTexCoord2f(1.0f, 0.0f); glVertex3f(px + right_x * radius - real_up_x * radius, py + right_y * radius - real_up_y * radius, pz + right_z * radius - real_up_z * radius);
    glTexCoord2f(1.0f, 1.0f); glVertex3f(px + right_x * radius + real_up_x * radius, py + right_y * radius + real_up_y * radius, pz + right_z * radius + real_up_z * radius);
    glTexCoord2f(0.0f, 1.0f); glVertex3f(px - right_x * radius + real_up_x * radius, py - right_y * radius + real_up_y * radius, pz - right_z * radius + real_up_z * radius);
    glEnd();

    retro_material_restore(&prev);
}

int retro_sky_init(RetroSky *sky) {
    if (!sky) return 0;
    memset(sky, 0, sizeof(*sky));

    if (!proc_tex_create(&sky->cloud_tex, 128, 128)) return 0;
    fill_cloud_texture(&sky->cloud_tex);
    proctex_upload_to_gl(&sky->cloud_tex);

    if (!proc_tex_create(&sky->sun_tex, 64, 64)) {
        retro_sky_shutdown(sky);
        return 0;
    }
    fill_disc_texture(&sky->sun_tex, 1.0f, 0.92f, 0.67f, 0.03f);
    proctex_upload_to_gl(&sky->sun_tex);

    if (!proc_tex_create(&sky->moon_tex, 64, 64)) {
        retro_sky_shutdown(sky);
        return 0;
    }
    fill_disc_texture(&sky->moon_tex, 0.80f, 0.87f, 1.0f, 0.07f);
    proctex_upload_to_gl(&sky->moon_tex);

    sky->initialized = 1;
    return 1;
}

void retro_sky_shutdown(RetroSky *sky) {
    if (!sky) return;
    proc_tex_destroy(&sky->cloud_tex);
    proc_tex_destroy(&sky->sun_tex);
    proc_tex_destroy(&sky->moon_tex);
    sky->initialized = 0;
}

void retro_sky_draw(RetroSky *sky, float cam_x, float cam_y, float cam_z, float time_sec) {
    if (!sky || !sky->initialized) return;

    /*
     * We draw with depth testing/writes disabled so sky never intersects world.
     * State is restored afterwards to avoid leaking into terrain/map/entity passes.
     */
    GLboolean prev_depth_test = glIsEnabled(GL_DEPTH_TEST);
    GLboolean prev_cull = glIsEnabled(GL_CULL_FACE);
    GLboolean prev_lighting = glIsEnabled(GL_LIGHTING);
    GLboolean prev_blend = glIsEnabled(GL_BLEND);
    GLboolean prev_texture_2d = glIsEnabled(GL_TEXTURE_2D);
    GLboolean prev_depth_write;
    glGetBooleanv(GL_DEPTH_WRITEMASK, &prev_depth_write);

    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);
    glDisable(GL_LIGHTING);
    glDisable(GL_CULL_FACE);

    glPushMatrix();
    glTranslatef(cam_x, cam_y, cam_z);

    draw_sky_cube(3200.0f);

    float cloud_scroll = fracf(time_sec * 0.0016f);
    draw_cloud_layer(sky->cloud_tex.tex_id, 1800.0f, 940.0f, cloud_scroll, 0.22f);
    draw_cloud_layer(sky->cloud_tex.tex_id, 1500.0f, 880.0f, fracf(-time_sec * 0.0011f), 0.16f);

    /*
     * Sun/moon move on one tilted circular orbit and are always opposite vectors.
     * This gives a simple, stable day-night motion suitable for a retro style.
     */
    float sun_dir_x = 0.0f, sun_dir_y = 0.0f, sun_dir_z = 0.0f;
    retro_eval_sun_dir(time_sec, &sun_dir_x, &sun_dir_y, &sun_dir_z);
    float sun_dist = 2200.0f;
    float moon_dist = 2160.0f;

    draw_orb_billboard(sky->sun_tex.tex_id,
                       sun_dir_x * sun_dist,
                       sun_dir_y * sun_dist,
                       sun_dir_z * sun_dist,
                       150.0f,
                       0.86f);
    draw_orb_billboard(sky->moon_tex.tex_id,
                       -sun_dir_x * moon_dist,
                       -sun_dir_y * moon_dist,
                       -sun_dir_z * moon_dist,
                       115.0f,
                       0.66f);

    glPopMatrix();

    if (prev_depth_test) glEnable(GL_DEPTH_TEST); else glDisable(GL_DEPTH_TEST);
    glDepthMask(prev_depth_write);
    if (prev_cull) glEnable(GL_CULL_FACE); else glDisable(GL_CULL_FACE);
    if (prev_lighting) glEnable(GL_LIGHTING); else glDisable(GL_LIGHTING);
    if (prev_blend) glEnable(GL_BLEND); else glDisable(GL_BLEND);
    if (prev_texture_2d) glEnable(GL_TEXTURE_2D); else glDisable(GL_TEXTURE_2D);
}
