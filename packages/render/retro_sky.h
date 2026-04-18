#ifndef RETRO_SKY_H
#define RETRO_SKY_H

#include "proc_tex.h"

typedef struct RetroSky {
    int initialized;
    ProcTexture cloud_tex;
    ProcTexture sun_tex;
    ProcTexture moon_tex;
} RetroSky;

int retro_sky_init(RetroSky *sky);
void retro_sky_shutdown(RetroSky *sky);

/*
 * Sky is rendered camera-centered so it appears infinitely far away:
 * camera translation is canceled while camera rotation remains, which keeps
 * parallax out of the background while preserving look direction.
 */
void retro_sky_draw(RetroSky *sky, float cam_x, float cam_y, float cam_z, float time_sec);

#endif
