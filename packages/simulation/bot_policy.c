#include "bot_policy.h"

#include <math.h>
#include <string.h>

#include "neural_net.h"

static float bot_absf(float v) { return v < 0.0f ? -v : v; }

static float clampf(float v, float lo, float hi) {
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}

static float normalize_angle_deg(float deg) {
    while (deg > 180.0f) deg -= 360.0f;
    while (deg < -180.0f) deg += 360.0f;
    return deg;
}

static int server_team_mode_enabled(int mode) {
    return mode == MODE_TDM || mode == MODE_TDMB || mode == MODE_TDMO || mode == MODE_CTF || mode == MODE_CTFB || mode == MODE_CTFO;
}

int bot_policy_neural_weights_available(void) {
    float sentinel = bot_absf(l3_b[0]) + bot_absf(l3_b[1]) + bot_absf(l3_b[2]) + bot_absf(l3_b[3]);
    return sentinel > 0.00001f;
}

BotPolicyKind bot_policy_select_default(int allow_genome_fallback) {
    if (bot_policy_neural_weights_available()) return BOT_POLICY_NEURAL;
    if (allow_genome_fallback) return BOT_POLICY_GENOME;
    return BOT_POLICY_SCRIPTED;
}

int bot_policy_find_target(int bot_idx, const ServerState *world, int require_los) {
    const PlayerState *me = &world->players[bot_idx];
    int team_mode = server_team_mode_enabled(world->game_mode);
    float best_dist_sq = 1e30f;
    int target_idx = -1;

    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (i == bot_idx) continue;
        const PlayerState *p = &world->players[i];
        if (!p->active || p->state == STATE_DEAD) continue;
        if (p->scene_id != me->scene_id) continue;
        if (team_mode && p->team_id == me->team_id) continue;
        float dx = p->x - me->x;
        float dz = p->z - me->z;
        float d2 = dx * dx + dz * dz;
        if (d2 >= best_dist_sq) continue;

        if (require_los) {
            float yaw_rad = me->yaw * 0.0174532925f;
            float fwd_x = sinf(yaw_rad);
            float fwd_z = cosf(yaw_rad);
            float dist = sqrtf(d2);
            float inv = dist > 0.001f ? (1.0f / dist) : 0.0f;
            float dot = fwd_x * (dx * inv) + fwd_z * (dz * inv);
            if (dot < 0.1f) continue;
        }

        best_dist_sq = d2;
        target_idx = i;
    }

    return target_idx;
}

void build_bot_observation(int bot_idx, const ServerState *world, int target_idx, BotObservation *obs) {
    const PlayerState *me = &world->players[bot_idx];
    memset(obs, 0, sizeof(*obs));

    float yaw_rad = me->yaw * 0.0174532925f;
    float fwd_x = sinf(yaw_rad);
    float fwd_z = cosf(yaw_rad);

    if (target_idx >= 0 && target_idx < MAX_CLIENTS) {
        const PlayerState *t = &world->players[target_idx];
        float dx = t->x - me->x;
        float dz = t->z - me->z;
        float dist = sqrtf(dx * dx + dz * dz);
        if (dist > 0.001f) {
            float inv = 1.0f / dist;
            obs->rel_enemy_x = dx * inv;
            obs->rel_enemy_z = dz * inv;
            float dot = clampf(fwd_x * obs->rel_enemy_x + fwd_z * obs->rel_enemy_z, -1.0f, 1.0f);
            float aim_err = acosf(dot) * (180.0f / 3.14159265f);
            obs->aim_error_norm = clampf(aim_err / 180.0f, 0.0f, 1.0f);
        }
        obs->enemy_distance_norm = clampf(dist / 120.0f, 0.0f, 1.0f);
    } else {
        obs->enemy_distance_norm = 1.0f;
        obs->aim_error_norm = 1.0f;
    }

    obs->self_health_norm = clampf((float)me->health / 100.0f, 0.0f, 1.0f);
    if (me->current_weapon >= 0 && me->current_weapon < MAX_WEAPONS) {
        int ammo_max = WPN_STATS[me->current_weapon].ammo_max;
        float ammo_norm = (ammo_max > 0) ? ((float)me->ammo[me->current_weapon] / (float)ammo_max) : 0.0f;
        obs->ammo_pressure = clampf(1.0f - ammo_norm, 0.0f, 1.0f);
    }

    int allies = 0;
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (i == bot_idx) continue;
        const PlayerState *p = &world->players[i];
        if (!p->active || p->state == STATE_DEAD) continue;
        if (p->scene_id != me->scene_id) continue;
        if (p->team_id != me->team_id) continue;
        float dx = p->x - me->x;
        float dz = p->z - me->z;
        if ((dx * dx + dz * dz) < (20.0f * 20.0f)) allies++;
    }
    obs->team_crowding = clampf((float)allies / 4.0f, 0.0f, 1.0f);

    float danger = 0.0f;
    if (me->shield < 35) danger += 0.45f;
    if (me->health < 50) danger += 0.35f;
    if (me->state == STATE_DEAD) danger = 1.0f;
    obs->danger_signal = clampf(danger, 0.0f, 1.0f);
}

void pack_bot_observation_vec(const BotObservation *obs, float vec8[8]) {
    vec8[0] = obs->rel_enemy_x;
    vec8[1] = obs->rel_enemy_z;
    vec8[2] = obs->enemy_distance_norm;
    vec8[3] = obs->aim_error_norm;
    vec8[4] = obs->self_health_norm;
    vec8[5] = obs->ammo_pressure;
    vec8[6] = obs->team_crowding;
    vec8[7] = obs->danger_signal;
}

void bot_policy_scripted_eval(int bot_idx, const ServerState *world, int target_idx, const BotObservation *obs, BotPolicyRuntime *rt, BotAction *action) {
    (void)bot_idx;
    memset(action, 0, sizeof(*action));
    if (target_idx < 0) {
        action->forward = 0.35f;
        action->strafe = ((world->server_tick / 30) & 1) ? 0.5f : -0.5f;
        action->yaw_rate = 2.0f;
        return;
    }

    float aim = obs->aim_error_norm;
    float dist = obs->enemy_distance_norm;
    float turn_sign = (obs->rel_enemy_x >= 0.0f) ? 1.0f : -1.0f;
    action->yaw_rate = turn_sign * (2.0f + (1.0f - aim) * 6.0f);

    if (dist > 0.38f) action->forward = 1.0f;
    else if (dist < 0.12f) action->forward = -0.7f;
    else action->forward = 0.2f;

    int phase = (world->server_tick / 24) & 1;
    rt->strafe_phase = phase;
    action->strafe = phase ? 0.65f : -0.65f;

    if (obs->ammo_pressure > 0.95f) action->reload = 1;
    if (aim < 0.12f && dist < 0.75f && !action->reload) action->shoot = 1;
}

void bot_policy_genome_eval(int bot_idx, const ServerState *world, int target_idx, BotAction *action) {
    const PlayerState *me = &world->players[bot_idx];
    memset(action, 0, sizeof(*action));
    if (target_idx < 0) {
        action->forward = 0.5f;
        action->yaw_rate = 2.0f;
        return;
    }
    const PlayerState *t = &world->players[target_idx];
    float dx = t->x - me->x;
    float dz = t->z - me->z;
    float dist = sqrtf(dx * dx + dz * dz);
    float target_yaw = atan2f(dx, dz) * (180.0f / 3.14159f);
    float diff = normalize_angle_deg(target_yaw - me->yaw);
    float turn_speed = me->brain.w_turret > 1.0f ? me->brain.w_turret : 10.0f;
    action->yaw_rate = clampf(diff, -turn_speed, turn_speed);
    action->shoot = 1;
    if (dist > 15.0f) action->forward = me->brain.w_aggro;
    else if (dist < 5.0f) action->forward = -me->brain.w_aggro;
    else action->forward = 0.2f;
    action->strafe = me->brain.w_strafe;
    if (me->on_ground && me->ammo[me->current_weapon] <= 0) action->reload = 1;
}

void bot_policy_neural_eval(const BotObservation *obs, BotAction *action) {
    float in[8];
    float out[4];
    memset(action, 0, sizeof(*action));
    pack_bot_observation_vec(obs, in);
    bot_brain_forward(in, out);
    action->forward = clampf(out[0], -1.0f, 1.0f);
    action->strafe = clampf(out[1], -1.0f, 1.0f);
    action->yaw_rate = out[2] * 8.0f;
    action->shoot = out[3] > 0.45f;
}

void bot_policy_action_to_usercmd(const BotAction *action, float *out_fwd, float *out_strafe, float *out_yaw, int *out_buttons) {
    *out_fwd = action->forward;
    *out_strafe = action->strafe;
    *out_yaw += action->yaw_rate;
    if (action->shoot) *out_buttons |= BTN_ATTACK;
    if (action->jump) *out_buttons |= BTN_JUMP;
    if (action->crouch) *out_buttons |= BTN_CROUCH;
    if (action->reload) *out_buttons |= BTN_RELOAD;
}
