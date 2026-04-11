#include "terrain.h"

#include <stdlib.h>
#include <string.h>
#include <math.h>

static int terrain_index(const TerrainHeightfield *t, int gx, int gz) {
    return gz * t->width + gx;
}

static float terrain_get_height_clamped(const TerrainHeightfield *t, int gx, int gz) {
    if (!t || !t->active || !t->heights || t->width <= 0 || t->height <= 0) return 0.0f;
    if (gx < 0) gx = 0;
    if (gz < 0) gz = 0;
    if (gx >= t->width) gx = t->width - 1;
    if (gz >= t->height) gz = t->height - 1;
    return t->heights[terrain_index(t, gx, gz)];
}

int terrain_init(TerrainHeightfield *t, int width, int height, float cell_size, float origin_x, float origin_z) {
    if (!t || width <= 1 || height <= 1 || cell_size <= 0.0f) return 0;
    terrain_free(t);
    t->width = width;
    t->height = height;
    t->cell_size = cell_size;
    t->origin_x = origin_x;
    t->origin_z = origin_z;
    t->active = 0;
    t->heights = (float*)malloc((size_t)width * (size_t)height * sizeof(float));
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
    free(t->heights);
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
    int count = t->width * t->height;
    for (int i = 0; i < count; i++) {
        t->heights[i] = h;
    }
}

void terrain_set_height(TerrainHeightfield *t, int gx, int gz, float h) {
    if (!t || !t->active || !t->heights) return;
    if (gx < 0 || gz < 0 || gx >= t->width || gz >= t->height) return;
    t->heights[terrain_index(t, gx, gz)] = h;
}

float terrain_get_height(const TerrainHeightfield *t, int gx, int gz) {
    if (!t || !t->active || !t->heights) return 0.0f;
    if (gx < 0 || gz < 0 || gx >= t->width || gz >= t->height) return 0.0f;
    return t->heights[terrain_index(t, gx, gz)];
}

int terrain_contains_world(const TerrainHeightfield *t, float x, float z) {
    if (!t || !t->active || !t->heights || t->width < 2 || t->height < 2) return 0;
    float local_x = (x - t->origin_x) / t->cell_size;
    float local_z = (z - t->origin_z) / t->cell_size;
    return local_x >= 0.0f && local_z >= 0.0f &&
           local_x <= (float)(t->width - 1) && local_z <= (float)(t->height - 1);
}

float terrain_sample_height(const TerrainHeightfield *t, float x, float z) {
    if (!t || !t->active || !t->heights || t->width < 2 || t->height < 2) return 0.0f;

    float local_x = (x - t->origin_x) / t->cell_size;
    float local_z = (z - t->origin_z) / t->cell_size;

    int ix = (int)floorf(local_x);
    int iz = (int)floorf(local_z);
    float fx = local_x - (float)ix;
    float fz = local_z - (float)iz;

    float h00 = terrain_get_height_clamped(t, ix, iz);
    float h10 = terrain_get_height_clamped(t, ix + 1, iz);
    float h01 = terrain_get_height_clamped(t, ix, iz + 1);
    float h11 = terrain_get_height_clamped(t, ix + 1, iz + 1);

    float hx0 = h00 + (h10 - h00) * fx;
    float hx1 = h01 + (h11 - h01) * fx;
    return hx0 + (hx1 - hx0) * fz;
}

void terrain_sample_normal(const TerrainHeightfield *t, float x, float z, float *nx, float *ny, float *nz) {
    if (nx) *nx = 0.0f;
    if (ny) *ny = 1.0f;
    if (nz) *nz = 0.0f;
    if (!t || !t->active || !t->heights) return;

    const float step = (t->cell_size > 0.001f) ? t->cell_size : 1.0f;
    float h_l = terrain_sample_height(t, x - step, z);
    float h_r = terrain_sample_height(t, x + step, z);
    float h_d = terrain_sample_height(t, x, z - step);
    float h_u = terrain_sample_height(t, x, z + step);

    float sx = h_l - h_r;
    float sy = 2.0f * step;
    float sz = h_d - h_u;
    float len = sqrtf(sx * sx + sy * sy + sz * sz);
    if (len < 0.0001f) return;

    if (nx) *nx = sx / len;
    if (ny) *ny = sy / len;
    if (nz) *nz = sz / len;
}
