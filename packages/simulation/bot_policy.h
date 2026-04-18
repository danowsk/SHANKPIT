#ifndef BOT_POLICY_H
#define BOT_POLICY_H

#include "../common/protocol.h"
#include "../common/physics.h"
#include "neural_net.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

typedef enum {
    BOT_POLICY_SCRIPTED = 0,
    BOT_POLICY_GENOME = 1,
    BOT_POLICY_NEURAL = 2,
    BOT_POLICY_AUTO = 3
} BotPolicyKind;

typedef struct {
    float enemy_rel_forward;      // [-1,1]
    float enemy_rel_right;        // [-1,1]
    float enemy_distance_norm;    // [0,1]
    float aim_error_norm;         // [0,1]
    float self_health_norm;       // [0,1]
    float ammo_pressure;          // [0,1]
    float team_crowding_pressure; // [0,1]
    float danger_stuck_signal;    // [0,1]

    int has_enemy;
    int enemy_id;
    int enemy_visible;
    float enemy_world_distance;
} BotObservation;

typedef struct {
    float forward;
    float strafe;
    float yaw_rate;
    int shoot;
    int jump;
    int crouch;
    int reload;
    int use;
    int ability;
} BotAction;

static BotPolicyKind g_bot_policy_kind[MAX_CLIENTS];
static int g_bot_policy_logged_neural_fallback = 0;

static inline float bot_clamp01(float v) {
    if (v < 0.0f) return 0.0f;
    if (v > 1.0f) return 1.0f;
    return v;
}

static inline float bot_clamp11(float v) {
    if (v < -1.0f) return -1.0f;
    if (v > 1.0f) return 1.0f;
    return v;
}

static inline float bot_angle_diff(float a, float b) {
    float d = a - b;
    while (d > 180.0f) d -= 360.0f;
    while (d < -180.0f) d += 360.0f;
    return d;
}

static inline const char *bot_policy_kind_name(BotPolicyKind k) {
    switch (k) {
        case BOT_POLICY_SCRIPTED: return "SCRIPTED";
        case BOT_POLICY_GENOME: return "GENOME";
        case BOT_POLICY_NEURAL: return "NEURAL";
        case BOT_POLICY_AUTO: return "AUTO";
        default: return "UNKNOWN";
    }
}

static inline int bot_policy_weights_available(void) {
    return (sizeof(l1_w) / sizeof(l1_w[0]) == 2048) &&
           (sizeof(l1_b) / sizeof(l1_b[0]) == 256) &&
           (sizeof(l2_w) / sizeof(l2_w[0]) == 32768) &&
           (sizeof(l2_b) / sizeof(l2_b[0]) == 128) &&
           (sizeof(l3_w) / sizeof(l3_w[0]) == 512) &&
           (sizeof(l3_b) / sizeof(l3_b[0]) == 4);
}

static inline BotPolicyKind bot_policy_default_for_mode(int game_mode) {
    (void)game_mode;
    if (bot_policy_weights_available()) return BOT_POLICY_NEURAL;
    return BOT_POLICY_SCRIPTED;
}

static inline void bot_policy_set_for_player(int player_id, BotPolicyKind kind) {
    if (player_id < 0 || player_id >= MAX_CLIENTS) return;
    if (kind == BOT_POLICY_AUTO) kind = bot_policy_default_for_mode(MODE_TDM);
    if (kind == BOT_POLICY_NEURAL && !bot_policy_weights_available()) {
        kind = BOT_POLICY_SCRIPTED;
        if (!g_bot_policy_logged_neural_fallback) {
            printf("[BOT_POLICY] Neural weights unavailable/invalid, using scripted baseline.\n");
            g_bot_policy_logged_neural_fallback = 1;
        }
    }
    g_bot_policy_kind[player_id] = kind;
}

static inline int bot_can_target(const PlayerState *me, const PlayerState *other, const ServerState *world) {
    if (!other->active || other->state == STATE_DEAD) return 0;
    if (other->scene_id != me->scene_id) return 0;
    int team_mode = (world->game_mode == MODE_TDM || world->game_mode == MODE_CTF ||
                     world->game_mode == MODE_TDMB || world->game_mode == MODE_TDMO || world->game_mode == MODE_CTFB);
    if (team_mode && other->team_id == me->team_id) return 0;
    return 1;
}

static inline int bot_line_of_sight(const PlayerState *me, const PlayerState *target) {
    float hit_x = 0.0f, hit_y = 0.0f, hit_z = 0.0f;
    float nx = 0.0f, ny = 0.0f, nz = 0.0f;
    phys_set_scene(me->scene_id);
    int blocked = trace_map(me->x, me->y + EYE_HEIGHT, me->z,
                            target->x, target->y + EYE_HEIGHT, target->z,
                            &hit_x, &hit_y, &hit_z, &nx, &ny, &nz);
    return blocked ? 0 : 1;
}

static inline void build_bot_observation(int bot_idx, const ServerState *world, BotObservation *obs) {
    memset(obs, 0, sizeof(*obs));
    const PlayerState *me = &world->players[bot_idx];

    int best = -1;
    float best_d2 = 1e30f;
    float nearest_ally_d2 = 1e30f;

    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (i == bot_idx) continue;
        const PlayerState *p = &world->players[i];
        if (!p->active || p->state == STATE_DEAD || p->scene_id != me->scene_id) continue;
        float dx = p->x - me->x;
        float dz = p->z - me->z;
        float d2 = dx*dx + dz*dz;
        if (p->team_id == me->team_id) {
            if (d2 < nearest_ally_d2) nearest_ally_d2 = d2;
            continue;
        }
        if (!bot_can_target(me, p, world)) continue;
        if (d2 < best_d2) {
            best_d2 = d2;
            best = i;
        }
    }

    if (best != -1) {
        const PlayerState *enemy = &world->players[best];
        float dx = enemy->x - me->x;
        float dz = enemy->z - me->z;
        float dist = sqrtf(best_d2);
        float yaw_rad = me->yaw * 0.0174532925f;
        float fwd_x = sinf(yaw_rad);
        float fwd_z = cosf(yaw_rad);
        float right_x = cosf(yaw_rad);
        float right_z = -sinf(yaw_rad);
        float inv_dist = (dist > 0.0001f) ? (1.0f / dist) : 0.0f;

        obs->has_enemy = 1;
        obs->enemy_id = best;
        obs->enemy_world_distance = dist;
        obs->enemy_rel_forward = bot_clamp11((dx * fwd_x + dz * fwd_z) * inv_dist);
        obs->enemy_rel_right = bot_clamp11((dx * right_x + dz * right_z) * inv_dist);
        obs->enemy_distance_norm = bot_clamp01(dist / 80.0f);

        float enemy_yaw = atan2f(dx, dz) * (180.0f / 3.14159265f);
        float aim_err = fabsf(bot_angle_diff(enemy_yaw, me->yaw));
        obs->aim_error_norm = bot_clamp01(aim_err / 90.0f);
        obs->enemy_visible = bot_line_of_sight(me, enemy);
    }

    obs->self_health_norm = bot_clamp01((float)me->health / 100.0f);
    int weapon = (me->current_weapon >= 0 && me->current_weapon < MAX_WEAPONS) ? me->current_weapon : 0;
    int ammo_max = WPN_STATS[weapon].ammo_max;
    if (ammo_max > 0) {
        obs->ammo_pressure = bot_clamp01(1.0f - ((float)me->ammo[weapon] / (float)ammo_max));
    }

    if (nearest_ally_d2 < 1e29f) {
        float ally_dist = sqrtf(nearest_ally_d2);
        obs->team_crowding_pressure = bot_clamp01((15.0f - ally_dist) / 15.0f);
    }

    float speed = sqrtf(me->vx * me->vx + me->vz * me->vz);
    float stuck = (speed < 0.05f && me->on_ground) ? 1.0f : 0.0f;
    float danger = (me->hit_feedback > 0) ? bot_clamp01((float)me->hit_feedback / 16.0f) : 0.0f;
    if (me->shield < 25) danger = bot_clamp01(danger + 0.25f);
    obs->danger_stuck_signal = bot_clamp01(0.6f * danger + 0.4f * stuck);
}

static inline void pack_bot_observation_vec(const BotObservation *obs, float out[8]) {
    out[0] = obs->enemy_rel_forward;
    out[1] = obs->enemy_rel_right;
    out[2] = obs->enemy_distance_norm;
    out[3] = obs->aim_error_norm;
    out[4] = obs->self_health_norm;
    out[5] = obs->ammo_pressure;
    out[6] = obs->team_crowding_pressure;
    out[7] = obs->danger_stuck_signal;
}

static inline void bot_policy_eval_scripted(int bot_idx, const ServerState *world, const BotObservation *obs, BotAction *act) {
    const PlayerState *me = &world->players[bot_idx];
    memset(act, 0, sizeof(*act));
    int cadence = ((world->server_tick / 45) + bot_idx) & 1;
    act->strafe = cadence ? 0.6f : -0.6f;

    if (!obs->has_enemy) {
        act->forward = 0.5f;
        act->yaw_rate = cadence ? 2.0f : -2.0f;
        return;
    }

    const PlayerState *enemy = &world->players[obs->enemy_id];
    float dx = enemy->x - me->x;
    float dz = enemy->z - me->z;
    float target_yaw = atan2f(dx, dz) * (180.0f / 3.14159265f);
    float yaw_err = bot_angle_diff(target_yaw, me->yaw);

    float yaw_rate = yaw_err * 0.22f;
    if (yaw_rate > 9.0f) yaw_rate = 9.0f;
    if (yaw_rate < -9.0f) yaw_rate = -9.0f;
    act->yaw_rate = yaw_rate;

    float d = obs->enemy_world_distance;
    if (d > 22.0f) act->forward = 1.0f;
    else if (d < 8.5f) act->forward = -0.55f;
    else act->forward = 0.2f;

    if (obs->danger_stuck_signal > 0.75f && ((world->server_tick + bot_idx) % 60 == 0)) {
        act->jump = 1;
        act->forward = 0.9f;
    }

    if (obs->enemy_visible && fabsf(yaw_err) < 10.0f && d < 90.0f) act->shoot = 1;
    if (me->current_weapon >= 0 && me->current_weapon < MAX_WEAPONS &&
        WPN_STATS[me->current_weapon].ammo_max > 0 && me->ammo[me->current_weapon] <= 0) {
        act->reload = 1;
    }
}

static inline void bot_policy_eval_genome_legacy(int bot_idx, const ServerState *world, const BotObservation *obs, BotAction *act) {
    const PlayerState *me = &world->players[bot_idx];
    memset(act, 0, sizeof(*act));

    if (!obs->has_enemy) {
        act->forward = 0.5f;
        act->yaw_rate = 2.0f;
        return;
    }

    const PlayerState *enemy = &world->players[obs->enemy_id];
    float dx = enemy->x - me->x;
    float dz = enemy->z - me->z;
    float target_yaw = atan2f(dx, dz) * (180.0f / 3.14159265f);
    float diff = bot_angle_diff(target_yaw, me->yaw);
    float turn_speed = (me->brain.w_turret > 1.0f) ? me->brain.w_turret : 10.0f;
    if (diff > turn_speed) diff = turn_speed;
    if (diff < -turn_speed) diff = -turn_speed;
    act->yaw_rate = diff + me->brain.w_strafe * 4.0f;

    if (obs->enemy_world_distance > 15.0f) act->forward = me->brain.w_aggro;
    else if (obs->enemy_world_distance < 5.0f) act->forward = -me->brain.w_aggro;
    else act->forward = 0.2f;

    act->shoot = 1;
    if (me->ammo[me->current_weapon] <= 0) act->reload = 1;
}

static inline void bot_policy_eval_neural(const BotObservation *obs, BotAction *act) {
    float in[8];
    float out[4];
    pack_bot_observation_vec(obs, in);
    bot_brain_forward(in, out);
    memset(act, 0, sizeof(*act));
    act->forward = bot_clamp11(out[0]);
    act->strafe = bot_clamp11(out[1]);
    act->yaw_rate = out[2] * 12.0f;
    act->shoot = (out[3] > 0.35f) ? 1 : 0;
}

static inline void bot_action_to_usercmd(const BotAction *act, float *out_fwd, float *out_yaw, int *out_buttons) {
    *out_fwd = bot_clamp11(act->forward);
    *out_buttons = 0;
    if (act->shoot) *out_buttons |= BTN_ATTACK;
    if (act->jump) *out_buttons |= BTN_JUMP;
    if (act->crouch) *out_buttons |= BTN_CROUCH;
    if (act->reload) *out_buttons |= BTN_RELOAD;
    if (act->use) *out_buttons |= BTN_USE;
    if (act->ability) *out_buttons |= BTN_ABILITY_1;
    *out_yaw += act->yaw_rate;
}

static inline void bot_policy_think(int bot_idx, ServerState *world, float *out_fwd, float *out_yaw, int *out_buttons,
                                    BotObservation *out_obs, BotAction *out_action) {
    PlayerState *me = &world->players[bot_idx];
    if (me->state == STATE_DEAD || world->match_over) {
        *out_fwd = 0.0f;
        *out_buttons = 0;
        if (out_obs) memset(out_obs, 0, sizeof(*out_obs));
        if (out_action) memset(out_action, 0, sizeof(*out_action));
        return;
    }

    BotObservation obs;
    BotAction act;
    build_bot_observation(bot_idx, world, &obs);

    BotPolicyKind kind = g_bot_policy_kind[bot_idx];
    if (kind == BOT_POLICY_AUTO) kind = bot_policy_default_for_mode(world->game_mode);
    if (kind == BOT_POLICY_NEURAL && !bot_policy_weights_available()) kind = BOT_POLICY_SCRIPTED;

    switch (kind) {
        case BOT_POLICY_NEURAL:
            bot_policy_eval_neural(&obs, &act);
            break;
        case BOT_POLICY_GENOME:
            bot_policy_eval_genome_legacy(bot_idx, world, &obs, &act);
            break;
        case BOT_POLICY_SCRIPTED:
        default:
            bot_policy_eval_scripted(bot_idx, world, &obs, &act);
            break;
    }

    bot_action_to_usercmd(&act, out_fwd, out_yaw, out_buttons);
    if (out_obs) *out_obs = obs;
    if (out_action) *out_action = act;
}

#endif
