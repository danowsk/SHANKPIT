#include "retro_lighting.h"

#include <math.h>
#include <string.h>

#include "retro_sky.h"

static float clamp01f(float v) {
    if (v < 0.0f) return 0.0f;
    if (v > 1.0f) return 1.0f;
    return v;
}

static float smoothstepf(float edge0, float edge1, float x) {
    float t = clamp01f((x - edge0) / (edge1 - edge0));
    return t * t * (3.0f - 2.0f * t);
}

static float saturate_dir_dot(float nx, float ny, float nz, float lx, float ly, float lz) {
    float d = nx * lx + ny * ly + nz * lz;
    if (d < 0.0f) return 0.0f;
    return d;
}

static void normalize3(float *x, float *y, float *z) {
    float len = sqrtf((*x) * (*x) + (*y) * (*y) + (*z) * (*z));
    if (len < 0.0001f) {
        *x = 0.0f;
        *y = 1.0f;
        *z = 0.0f;
        return;
    }
    *x /= len;
    *y /= len;
    *z /= len;
}

const char *retro_lighting_preset_name(RetroLightingPreset preset) {
    switch (preset) {
        case RETRO_LIGHTING_DYNAMIC: return "dynamic";
        case RETRO_LIGHTING_DAY_STATIC: return "day_static";
        case RETRO_LIGHTING_SUNSET_STATIC: return "sunset_static";
        case RETRO_LIGHTING_NIGHT_MOONLIT: return "night_moonlit";
        case RETRO_LIGHTING_INTERIOR_FLAT: return "interior_flat";
        default: return "unknown";
    }
}

void retro_lighting_eval(float time_sec, RetroLightingPreset preset, RetroLightingState *out_state) {
    RetroLightingState s;
    memset(&s, 0, sizeof(s));
    s.preset = preset;

    s.fog_near = 420.0f;
    s.fog_far = 2800.0f;
    s.moon_r = 0.55f;
    s.moon_g = 0.64f;
    s.moon_b = 0.92f;

    if (preset == RETRO_LIGHTING_DYNAMIC) {
        retro_sky_eval_sun_dir(time_sec, &s.sun_dir_x, &s.sun_dir_y, &s.sun_dir_z);
        normalize3(&s.sun_dir_x, &s.sun_dir_y, &s.sun_dir_z);

        s.moon_dir_x = -s.sun_dir_x;
        s.moon_dir_y = -s.sun_dir_y;
        s.moon_dir_z = -s.sun_dir_z;

        {
            float sun_visibility = smoothstepf(0.0f, 0.22f, s.sun_dir_y);
            float moon_visibility = smoothstepf(0.0f, 0.28f, s.moon_dir_y) * (1.0f - sun_visibility);
            s.sun_r = 1.0f;
            s.sun_g = 0.93f;
            s.sun_b = 0.82f;
            s.sun_intensity = 0.82f * sun_visibility;
            s.allow_moonlight = 1;
            s.moon_intensity = 0.12f * moon_visibility;
            s.ambient_r = 0.40f + sun_visibility * 0.18f;
            s.ambient_g = 0.44f + sun_visibility * 0.18f;
            s.ambient_b = 0.50f + sun_visibility * 0.14f;
            s.ambient_intensity = 0.78f - sun_visibility * 0.18f;
        }

        retro_sky_eval_fog_rgb(time_sec, &s.fog_r, &s.fog_g, &s.fog_b);
    } else if (preset == RETRO_LIGHTING_DAY_STATIC) {
        s.ambient_r = 0.56f; s.ambient_g = 0.58f; s.ambient_b = 0.58f; s.ambient_intensity = 0.72f;
        s.fog_r = 0.46f; s.fog_g = 0.57f; s.fog_b = 0.68f;
        s.sun_dir_x = 0.32f; s.sun_dir_y = 0.88f; s.sun_dir_z = 0.35f;
        normalize3(&s.sun_dir_x, &s.sun_dir_y, &s.sun_dir_z);
        s.sun_r = 1.0f; s.sun_g = 0.95f; s.sun_b = 0.86f; s.sun_intensity = 0.75f;
        s.moon_dir_x = -s.sun_dir_x; s.moon_dir_y = -s.sun_dir_y; s.moon_dir_z = -s.sun_dir_z;
        s.allow_moonlight = 0;
    } else if (preset == RETRO_LIGHTING_SUNSET_STATIC) {
        s.ambient_r = 0.64f; s.ambient_g = 0.52f; s.ambient_b = 0.44f; s.ambient_intensity = 0.70f;
        s.fog_r = 0.58f; s.fog_g = 0.45f; s.fog_b = 0.38f;
        s.sun_dir_x = 0.76f; s.sun_dir_y = 0.24f; s.sun_dir_z = 0.47f;
        normalize3(&s.sun_dir_x, &s.sun_dir_y, &s.sun_dir_z);
        s.sun_r = 1.0f; s.sun_g = 0.72f; s.sun_b = 0.50f; s.sun_intensity = 0.54f;
        s.moon_dir_x = -s.sun_dir_x; s.moon_dir_y = -s.sun_dir_y; s.moon_dir_z = -s.sun_dir_z;
        s.allow_moonlight = 0;
    } else if (preset == RETRO_LIGHTING_NIGHT_MOONLIT) {
        s.ambient_r = 0.30f; s.ambient_g = 0.36f; s.ambient_b = 0.47f; s.ambient_intensity = 0.70f;
        s.fog_r = 0.11f; s.fog_g = 0.16f; s.fog_b = 0.24f;
        s.sun_dir_x = 0.0f; s.sun_dir_y = 1.0f; s.sun_dir_z = 0.0f;
        s.sun_r = 1.0f; s.sun_g = 0.93f; s.sun_b = 0.82f; s.sun_intensity = 0.0f;
        s.moon_dir_x = -0.55f; s.moon_dir_y = 0.62f; s.moon_dir_z = -0.56f;
        normalize3(&s.moon_dir_x, &s.moon_dir_y, &s.moon_dir_z);
        s.allow_moonlight = 1;
        s.moon_intensity = 0.16f;
    } else {
        s.ambient_r = 0.52f; s.ambient_g = 0.52f; s.ambient_b = 0.53f; s.ambient_intensity = 0.76f;
        s.fog_r = 0.36f; s.fog_g = 0.38f; s.fog_b = 0.42f;
        s.fog_near = 520.0f;
        s.fog_far = 2200.0f;
        s.sun_dir_x = 0.0f; s.sun_dir_y = 1.0f; s.sun_dir_z = 0.0f;
        s.sun_r = 1.0f; s.sun_g = 1.0f; s.sun_b = 1.0f; s.sun_intensity = 0.0f;
        s.moon_dir_x = 0.0f; s.moon_dir_y = 1.0f; s.moon_dir_z = 0.0f;
        s.allow_moonlight = 0;
        s.moon_intensity = 0.0f;
    }

    if (s.sun_dir_y <= 0.0f) {
        s.sun_intensity = 0.0f;
    }
    if (!s.allow_moonlight || s.moon_dir_y <= 0.0f) {
        s.moon_intensity = 0.0f;
    }

    if (out_state) {
        *out_state = s;
    }
}

void retro_lighting_eval_surface_rgb(const RetroLightingState *state,
                                     float nx, float ny, float nz,
                                     float ambient_floor,
                                     float *out_r, float *out_g, float *out_b) {
    float ar = 1.0f;
    float ag = 1.0f;
    float ab = 1.0f;

    if (state) {
        float floor = clamp01f(ambient_floor);
        float sun_ndotl = saturate_dir_dot(nx, ny, nz, state->sun_dir_x, state->sun_dir_y, state->sun_dir_z);
        float moon_ndotl = saturate_dir_dot(nx, ny, nz, state->moon_dir_x, state->moon_dir_y, state->moon_dir_z);

        ar = state->ambient_r * state->ambient_intensity * floor;
        ag = state->ambient_g * state->ambient_intensity * floor;
        ab = state->ambient_b * state->ambient_intensity * floor;

        ar += state->sun_r * state->sun_intensity * sun_ndotl;
        ag += state->sun_g * state->sun_intensity * sun_ndotl;
        ab += state->sun_b * state->sun_intensity * sun_ndotl;

        ar += state->moon_r * state->moon_intensity * moon_ndotl;
        ag += state->moon_g * state->moon_intensity * moon_ndotl;
        ab += state->moon_b * state->moon_intensity * moon_ndotl;

        ar = clamp01f(ar);
        ag = clamp01f(ag);
        ab = clamp01f(ab);
    }

    if (out_r) *out_r = ar;
    if (out_g) *out_g = ag;
    if (out_b) *out_b = ab;
}
