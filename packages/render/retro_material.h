#ifndef RETRO_MATERIAL_H
#define RETRO_MATERIAL_H

#include <SDL2/SDL_opengl.h>

typedef struct RetroMaterial {
    GLuint tex_id;
    float color[4];
    int enable_blend;
    GLenum blend_src;
    GLenum blend_dst;
} RetroMaterial;

typedef struct RetroMaterialState {
    GLboolean texture_2d_enabled;
    GLboolean blend_enabled;
    GLint bound_tex_2d;
    GLfloat color[4];
} RetroMaterialState;

void retro_material_apply(const RetroMaterial *mat, RetroMaterialState *out_prev_state);
void retro_material_restore(const RetroMaterialState *prev_state);

#endif
