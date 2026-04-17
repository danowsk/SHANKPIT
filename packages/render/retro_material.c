#include "retro_material.h"

void retro_material_apply(const RetroMaterial *mat, RetroMaterialState *out_prev_state) {
    if (!mat || !out_prev_state) return;

    out_prev_state->texture_2d_enabled = glIsEnabled(GL_TEXTURE_2D);
    out_prev_state->blend_enabled = glIsEnabled(GL_BLEND);
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &out_prev_state->bound_tex_2d);
    glGetFloatv(GL_CURRENT_COLOR, out_prev_state->color);

    if (mat->tex_id != 0) {
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, mat->tex_id);
    } else {
        glDisable(GL_TEXTURE_2D);
    }

    if (mat->enable_blend) {
        glEnable(GL_BLEND);
        glBlendFunc(mat->blend_src, mat->blend_dst);
    } else {
        glDisable(GL_BLEND);
    }

    glColor4f(mat->color[0], mat->color[1], mat->color[2], mat->color[3]);
}

void retro_material_restore(const RetroMaterialState *prev_state) {
    if (!prev_state) return;

    if (prev_state->texture_2d_enabled) glEnable(GL_TEXTURE_2D);
    else glDisable(GL_TEXTURE_2D);

    if (prev_state->blend_enabled) glEnable(GL_BLEND);
    else glDisable(GL_BLEND);

    glBindTexture(GL_TEXTURE_2D, (GLuint)prev_state->bound_tex_2d);
    glColor4f(prev_state->color[0], prev_state->color[1], prev_state->color[2], prev_state->color[3]);
}
