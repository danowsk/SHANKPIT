#include "terrain.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

static int terrain_index(const TerrainHeightfield *t, int gx, int gz) {
    return gz * t->width + gx;
}

static float clampf_local(float v, float mn, float mx) {
    if (v < mn) return mn;
    if (v > mx) return mx;
    return v;
}

static float lerp_local(float a, float b, float t) {
    return a + (b - a) * t;
}

int terrain_init(TerrainHeightfield *t, int width, int height, float cell_size, float origin_x, float origin_z) {
    if (!t || width < 2 || height < 2 || cell_size <= 0.0f) return 0;

    terrain_free(t);

    t->width = width;
    t->height = height;
    t->cell_size = cell_size;
    t->origin_x = origin_x;
    t->origin_z = origin_z;
    t->active = 0;
    t->heights = (float *)malloc((size_t)width * (size_t)height * sizeof(float));
    if (!t->heights) {
        t->width = 0;
        t->height = 0;
        t->cell_size = 1.0f;
        return 0;
    }

    memset(t->heights, 0, (size_t)width * (size_t)height * sizeof(float));
    t->active = 1;
    return 1;
}

void terrain_free(TerrainHeightfield *t) {
    if (!t) return;
    if (t->heights) {
        free(t->heights);
    }
    t->heights = NULL;
    t->width = 0;
    t->height = 0;
    t->cell_size = 1.0f;
    t->origin_x = 0.0f;
    t->origin_z = 0.0f;
    t->active = 0;
}

void terrain_clear(TerrainHeightfield *t, float h) {
    if (!t || !t->heights) return;
    for (int gz = 0; gz < t->height; gz++) {
        for (int gx = 0; gx < t->width; gx++) {
            t->heights[terrain_index(t, gx, gz)] = h;
        }
    }
}

void terrain_set_height(TerrainHeightfield *t, int gx, int gz, float h) {
    if (!t || !t->heights) return;
    if (gx < 0 || gz < 0 || gx >= t->width || gz >= t->height) return;
    t->heights[terrain_index(t, gx, gz)] = h;
}

float terrain_get_height(const TerrainHeightfield *t, int gx, int gz) {
    if (!t || !t->heights || gx < 0 || gz < 0 || gx >= t->width || gz >= t->height) return 0.0f;
    return t->heights[terrain_index(t, gx, gz)];
}

int terrain_contains_world(const TerrainHeightfield *t, float x, float z) {
    if (!t || !t->active || !t->heights) return 0;
    if (x < t->origin_x || z < t->origin_z) return 0;

    const float max_x = t->origin_x + (float)(t->width - 1) * t->cell_size;
    const float max_z = t->origin_z + (float)(t->height - 1) * t->cell_size;
    if (x > max_x || z > max_z) return 0;
    return 1;
}

float terrain_sample_height(const TerrainHeightfield *t, float x, float z) {
    if (!t || !t->active || !t->heights || t->width < 2 || t->height < 2) return 0.0f;

    float local_x = (x - t->origin_x) / t->cell_size;
    float local_z = (z - t->origin_z) / t->cell_size;

    local_x = clampf_local(local_x, 0.0f, (float)(t->width - 1));
    local_z = clampf_local(local_z, 0.0f, (float)(t->height - 1));

    int gx0 = (int)floorf(local_x);
    int gz0 = (int)floorf(local_z);

    if (gx0 >= t->width - 1) gx0 = t->width - 2;
    if (gz0 >= t->height - 1) gz0 = t->height - 2;

    int gx1 = gx0 + 1;
    int gz1 = gz0 + 1;

    float tx = local_x - (float)gx0;
    float tz = local_z - (float)gz0;

    float h00 = terrain_get_height(t, gx0, gz0);
    float h10 = terrain_get_height(t, gx1, gz0);
    float h01 = terrain_get_height(t, gx0, gz1);
    float h11 = terrain_get_height(t, gx1, gz1);

    float hx0 = lerp_local(h00, h10, tx);
    float hx1 = lerp_local(h01, h11, tx);
    return lerp_local(hx0, hx1, tz);
}

void terrain_sample_normal(const TerrainHeightfield *t, float x, float z, float *nx, float *ny, float *nz) {
    if (!nx || !ny || !nz) return;

    if (!t || !t->active || !t->heights) {
        *nx = 0.0f; *ny = 1.0f; *nz = 0.0f;
        return;
    }

    float step = t->cell_size;
    float h_l = terrain_sample_height(t, x - step, z);
    float h_r = terrain_sample_height(t, x + step, z);
    float h_d = terrain_sample_height(t, x, z - step);
    float h_u = terrain_sample_height(t, x, z + step);

    float tx = h_l - h_r;
    float ty = 2.0f * step;
    float tz = h_d - h_u;

    float len = sqrtf(tx * tx + ty * ty + tz * tz);
    if (len < 0.0001f) {
        *nx = 0.0f; *ny = 1.0f; *nz = 0.0f;
        return;
    }

    *nx = tx / len;
    *ny = ty / len;
    *nz = tz / len;
}
