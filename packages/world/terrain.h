#ifndef SHANKPIT_TERRAIN_H
#define SHANKPIT_TERRAIN_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    int width;
    int height;
    float cell_size;
    float origin_x;
    float origin_z;
    float *heights;
    int active;
} TerrainHeightfield;

int terrain_init(TerrainHeightfield *t, int width, int height, float cell_size, float origin_x, float origin_z);
void terrain_free(TerrainHeightfield *t);
void terrain_clear(TerrainHeightfield *t, float h);
void terrain_set_height(TerrainHeightfield *t, int gx, int gz, float h);
float terrain_get_height(const TerrainHeightfield *t, int gx, int gz);
float terrain_sample_height(const TerrainHeightfield *t, float x, float z);
void terrain_sample_normal(const TerrainHeightfield *t, float x, float z, float *nx, float *ny, float *nz);
int terrain_contains_world(const TerrainHeightfield *t, float x, float z);

#ifdef __cplusplus
}
#endif

#endif
