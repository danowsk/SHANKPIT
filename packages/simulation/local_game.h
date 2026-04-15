#ifndef LOCAL_GAME_H
#define LOCAL_GAME_H

#include "../common/protocol.h"
#include "../common/physics.h"
#include "../common/shared_movement.h"
#include <string.h>

ServerState local_state;
int was_holding_jump = 0;
static int tdmb_last_kills[MAX_CLIENTS];
#define SHANKPIT_HELI_DEBUG 0
#define TDMB_BLUE_TEAM 1
#define TDMB_RED_TEAM 0
#define TDMB_BLUE_BOTS 5
#define TDMB_RED_BOTS 6
#define TDMB_SCORE_LIMIT 25

void local_update(float fwd, float str, float yaw, float pitch, int shoot, int weapon_req, int jump, int crouch, int reload, int ability, void *server_context, unsigned int cmd_time);
void update_entity(PlayerState *p, float dt, void *server_context, unsigned int cmd_time);
static inline void heli_spawn_defaults(HelicopterState *h, int id, int scene_id, float x, float y, float z);
void local_init_match(int num_players, int mode);

float rand_weight() { return ((float)(rand()%2000)/1000.0f) - 1.0f; } 
float rand_pos() { return ((float)(rand()%1000)/1000.0f); } 

void init_genome(BotGenome *g) {
    g->version = 1;
    g->w_aggro = 0.5f + rand_weight() * 0.5f;
    g->w_strafe = rand_weight();
    g->w_jump = 0.05f + rand_pos() * 0.1f; 
    g->w_slide = 0.01f + rand_pos() * 0.05f;
    g->w_turret = 5.0f + rand_pos() * 10.0f;
    g->w_repel = 1.0f + rand_pos();
}

void evolve_bot(PlayerState *loser, PlayerState *winner) {
    loser->brain = winner->brain;
    loser->brain.w_aggro += rand_weight() * 0.1f;
    loser->brain.w_strafe += rand_weight() * 0.1f;
    loser->brain.w_jump += rand_weight() * 0.01f;
    loser->brain.w_slide += rand_weight() * 0.01f;
}

PlayerState* get_best_bot() {
    PlayerState *best = NULL;
    float max_score = -99999.0f;
    for(int i=1; i<MAX_CLIENTS; i++) {
        if (!local_state.players[i].active) continue;
        if (local_state.players[i].accumulated_reward > max_score) {
            max_score = local_state.players[i].accumulated_reward;
            best = &local_state.players[i];
        }
    }
    return best;
}

static inline void scene_load(int scene_id) {
    local_state.scene_id = scene_id;
    phys_set_scene(scene_id);
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (!local_state.players[i].active) continue;
        local_state.players[i].scene_id = scene_id;
        scene_force_spawn(&local_state.players[i]);
    }
    for (int hi = 0; hi < MAX_HELICOPTERS; hi++) {
        memset(&local_state.helicopters[hi], 0, sizeof(local_state.helicopters[hi]));
        local_state.helicopters[hi].id = hi;
        local_state.helicopters[hi].occupant_player_id = -1;
    }

    if (scene_id == SCENE_VOXWORLD) {
        float red_y = voxworld_heli_spawn_y(VOXWORLD_BASE_RED_X);
        float blue_y = voxworld_heli_spawn_y(VOXWORLD_BASE_BLUE_X);
        heli_spawn_defaults(&local_state.helicopters[0], 0, SCENE_VOXWORLD, VOXWORLD_HELI_RED_X, red_y, VOXWORLD_HELI_RED_Z);
        heli_spawn_defaults(&local_state.helicopters[1], 1, SCENE_VOXWORLD, VOXWORLD_HELI_BLUE_X, blue_y, VOXWORLD_HELI_BLUE_Z);
        local_state.helicopters[0].yaw = 90.0f;
        local_state.helicopters[1].yaw = 270.0f;
        local_state.helicopters[0].grounded = 1;
        local_state.helicopters[1].grounded = 1;
        local_state.helicopters[0].rotor_speed = 14.0f;
        local_state.helicopters[1].rotor_speed = 14.0f;
#if SHANKPIT_HELI_DEBUG
        printf("[HELI] scene=%d spawned=2 red=(%.1f,%.1f,%.1f) blue=(%.1f,%.1f,%.1f)\n",
               scene_id,
               local_state.helicopters[0].x, local_state.helicopters[0].y, local_state.helicopters[0].z,
               local_state.helicopters[1].x, local_state.helicopters[1].y, local_state.helicopters[1].z);
#endif
    }
}

static inline void heli_spawn_defaults(HelicopterState *h, int id, int scene_id, float x, float y, float z) {
    memset(h, 0, sizeof(*h));
    h->active = 1;
    h->id = id;
    h->scene_id = scene_id;
    h->x = x; h->y = y; h->z = z;
    h->health = 250;
    h->occupant_player_id = -1;
    h->rotor_speed = 8.0f;
    h->yaw = 180.0f;
}

static inline HelicopterState *heli_find_nearby(int scene_id, float x, float y, float z, float radius) {
    float rr = radius * radius;
    for (int i = 0; i < MAX_HELICOPTERS; i++) {
        HelicopterState *h = &local_state.helicopters[i];
        if (!h->active || h->scene_id != scene_id) continue;
        float dx = h->x - x, dy = h->y - y, dz = h->z - z;
        if ((dx * dx + dy * dy + dz * dz) <= rr) return h;
    }
    return NULL;
}

static inline int heli_try_place_player(PlayerState *p, HelicopterState *h, float ox, float oz) {
    float px = h->x + ox;
    float pz = h->z + oz;
    if (!heli_point_collides(px, h->y + 1.0f, pz) && !heli_point_collides(px, h->y + 2.0f, pz)) {
        p->x = px; p->y = h->y; p->z = pz;
        return 1;
    }
    return 0;
}

static inline void scene_request_transition(int scene_id) {
    if (local_state.transition_timer > 0) return;
    local_state.pending_scene = scene_id;
    local_state.transition_timer = 12;
}

static inline void scene_tick_transition() {
    if (local_state.transition_timer <= 0) return;
    local_state.transition_timer--;
    if (local_state.transition_timer == 0 && local_state.pending_scene >= 0) {
        scene_load(local_state.pending_scene);
        local_state.pending_scene = -1;
    }
}

// --- BOT AI ---
void bot_think(int bot_idx, PlayerState *players, float *out_fwd, float *out_yaw, int *out_buttons) {
    PlayerState *me = &players[bot_idx];
    if (me->state == STATE_DEAD || local_state.match_over) { *out_buttons = 0; return; }

    int team_mode = (local_state.game_mode == MODE_TDM || local_state.game_mode == MODE_CTF || local_state.game_mode == MODE_TDMB);
    int target_idx = -1;
    float min_dist = 9999.0f;

    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (i == bot_idx) continue;
        if (!players[i].active) continue;
        if (players[i].state == STATE_DEAD) continue;
        if (team_mode && players[i].team_id == me->team_id) continue;
        
        float dx = players[i].x - me->x;
        float dz = players[i].z - me->z;
        float dist = sqrtf(dx*dx + dz*dz);
        
        if (i == 0 || dist < min_dist) { 
            if (i == 0 && (!team_mode || players[0].team_id != me->team_id)) dist *= 0.5f;
            if (dist < min_dist) { min_dist = dist; target_idx = i; }
        }
    }

    if (target_idx != -1) {
        PlayerState *t = &players[target_idx];
        float dx = t->x - me->x;
        float dz = t->z - me->z;
        float target_yaw = atan2f(dx, dz) * (180.0f / 3.14159f);
        
        float turn_speed = (me->brain.w_turret > 1.0f) ? me->brain.w_turret : 10.0f;
        float diff = angle_diff(target_yaw, *out_yaw);
        if (diff > turn_speed) diff = turn_speed;
        if (diff < -turn_speed) diff = -turn_speed;
        *out_yaw += diff;
        
        *out_buttons |= BTN_ATTACK;
        
        if (min_dist > 15.0f) *out_fwd = me->brain.w_aggro;
        else if (min_dist < 5.0f) *out_fwd = -me->brain.w_aggro; 
        else *out_fwd = 0.2f; 
        
        *out_yaw += me->brain.w_strafe * 10.0f;
        if (me->on_ground && (rand()%1000 < (me->brain.w_jump * 1000.0f))) *out_buttons |= BTN_JUMP;
        if (me->on_ground && (rand()%1000 < (me->brain.w_slide * 1000.0f))) *out_buttons |= BTN_CROUCH;
        if (me->ammo[me->current_weapon] <= 0) *out_buttons |= BTN_RELOAD;
    } else {
        *out_yaw += 2.0f;
        *out_fwd = 0.5f;
    }
}

// --- UPDATE LOOP ---
void update_entity(PlayerState *p, float dt, void *server_context, unsigned int cmd_time) {
    if (!p->active) return;
    if (p->state == STATE_DEAD) return;

    phys_set_scene(p->scene_id);

    if (cmd_time < p->stunned_until_ms) {
        p->in_fwd = 0.0f;
        p->in_strafe = 0.0f;
        p->in_jump = 0;
        p->in_shoot = 0;
        p->in_reload = 0;
        p->in_use = 0;
        p->in_ability = 0;
        p->vx = 0.0f;
        p->vz = 0.0f;
    }

    apply_friction(p);
    float g = (p->in_jump) ? GRAVITY_FLOAT : GRAVITY_DROP;
    if (p->dash_timer <= 0) p->vy -= g; 
    p->y += p->vy;
    
    resolve_collision(p);
    if (p->dash_timer > 0) {
        float nx = 0.0f, ny = 0.0f, nz = 0.0f;
        float hit_x = 0.0f, hit_y = 0.0f, hit_z = 0.0f;
        float next_x = p->x + p->vx;
        float next_y = p->y;
        float next_z = p->z + p->vz;
        if (trace_map(p->x, p->y + 1.0f, p->z, next_x, next_y + 1.0f, next_z, &hit_x, &hit_y, &hit_z, &nx, &ny, &nz)) {
            p->x = hit_x;
            p->z = hit_z;
            p->dash_timer = 0;
            p->dash_vx = p->dash_vy = p->dash_vz = 0.0f;
            p->vx = 0.0f; p->vy = 0.0f; p->vz = 0.0f;
        } else {
            p->x = next_x;
            p->z = next_z;
        }
    } else {
        p->x += p->vx;
        p->z += p->vz;
    }

    if (p->recoil_anim > 0) p->recoil_anim -= 0.1f;
    if (p->recoil_anim < 0) p->recoil_anim = 0;
    if (p->hit_feedback > 0) p->hit_feedback--;

    update_weapons(p, local_state.players, local_state.projectiles, p->in_shoot > 0, p->in_reload > 0, p->in_ability > 0);
    scene_safety_check(p);
}

static void apply_projectile_damage(PlayerState *owner, PlayerState *target, int damage, unsigned int now_ms) {
    if (!target->active || target->state == STATE_DEAD) return;
    int team_mode = (local_state.game_mode == MODE_TDM || local_state.game_mode == MODE_CTF || local_state.game_mode == MODE_TDMB);
    if (owner && team_mode && owner->team_id == target->team_id) return;
    target->shield_regen_timer = SHIELD_REGEN_DELAY;
    if (target->shield > 0) {
        if (target->shield >= damage) { target->shield -= damage; damage = 0; }
        else { damage -= target->shield; target->shield = 0; }
    }
    target->health -= damage;
    if (target->health <= 0) {
        if (owner) {
            owner->kills++;
            owner->accumulated_reward += 500.0f;
        }
        target->deaths++;
        phys_respawn(target, now_ms);
    }

    if (now_ms >= target->stun_immune_until_ms) {
        unsigned int stun_end = now_ms + 100;
        if (stun_end > target->stunned_until_ms) target->stunned_until_ms = stun_end;
        target->stun_immune_until_ms = now_ms + 250;
    }
}

static void update_projectiles(unsigned int now_ms) {
    for (int i=0; i<MAX_PROJECTILES; i++) {
        Projectile *p = &local_state.projectiles[i];
        if (!p->active) continue;

        phys_set_scene(p->scene_id);

        float next_x = p->x + p->vx;
        float next_y = p->y + p->vy;
        float next_z = p->z + p->vz;

        float hit_x, hit_y, hit_z, nx, ny, nz;
        if (trace_map(p->x, p->y, p->z, next_x, next_y, next_z, &hit_x, &hit_y, &hit_z, &nx, &ny, &nz)) {
            if (p->bounces_left > 0) {
                reflect_vector(&p->vx, &p->vy, &p->vz, nx, ny, nz);
                p->x = hit_x; p->y = hit_y; p->z = hit_z;
                p->bounces_left--;
            } else {
                p->active = 0;
            }
        } else {
            p->x = next_x; p->y = next_y; p->z = next_z;
        }

        if (p->active) {
            for (int t = 0; t < MAX_CLIENTS; t++) {
                PlayerState *target = &local_state.players[t];
                if (!target->active || target->state == STATE_DEAD) continue;
                if (t == p->owner_id) continue;
                if (target->scene_id != p->scene_id) continue;
                float dx = target->x - p->x;
                float dy = (target->y + EYE_HEIGHT) - p->y;
                float dz = target->z - p->z;
                float dist_sq = dx * dx + dy * dy + dz * dz;
                if (dist_sq < 4.0f) {
                    PlayerState *owner = NULL;
                    if (p->owner_id >= 0 && p->owner_id < MAX_CLIENTS) {
                        owner = &local_state.players[p->owner_id];
                    }
                    apply_projectile_damage(owner, target, p->damage, now_ms);
                    p->active = 0;
                    break;
                }
            }
        }

        if (p->x > 4000 || p->x < -4000 || p->z > 4000 || p->z < -4000 || p->y > 2000) p->active = 0;
    }
}

void local_update(float fwd, float str, float yaw, float pitch, int shoot, int weapon_req, int jump, int crouch, int reload, int ability, void *server_context, unsigned int cmd_time) {
    PlayerState *p0 = &local_state.players[0];
    if (local_state.match_over && local_state.game_mode == MODE_TDMB) {
        fwd = 0.0f; str = 0.0f; shoot = 0; jump = 0; crouch = 0; reload = 0; ability = 0;
    }
    scene_tick_transition();
    if (local_state.transition_timer > 0) {
        fwd = 0.0f;
        str = 0.0f;
        shoot = 0;
        jump = 0;
        crouch = 0;
        reload = 0;
        ability = 0;
    }
    p0->yaw = yaw; p0->pitch = pitch;
    if (weapon_req >= 0 && weapon_req < MAX_WEAPONS) p0->current_weapon = weapon_req;
    if (!(p0->in_vehicle && p0->vehicle_type == VEH_HELICOPTER)) {
        MoveIntent move_intent = {
            .forward = fwd,
            .strafe = str,
            .control_yaw_deg = yaw,
            .wants_jump = jump,
            .wants_sprint = 0
        };
        MoveWish move_wish = shankpit_move_wish_from_intent(move_intent);
        accelerate(p0, move_wish.dir_x, move_wish.dir_z, move_wish.magnitude * MAX_SPEED, ACCEL);
    }
    
    int fresh_jump_press = (jump && !was_holding_jump);
    // --- PHASE 485: TUNED SLIDE JUMP ---
    if (jump && p0->on_ground) {
        float speed = sqrtf(p0->vx*p0->vx + p0->vz*p0->vz);
        if (p0->crouching && speed > 0.5f && fresh_jump_press) {
            float boost_mult = 1.0f + (0.25f / speed);
            if (boost_mult > 1.4f) boost_mult = 1.4f;
            if (boost_mult < 1.02f) boost_mult = 1.02f;
            p0->vx *= boost_mult;
            p0->vz *= boost_mult;
        }
        p0->y += 0.1f;
        p0->vy += JUMP_FORCE;
    }
    p0->in_shoot = shoot; p0->in_reload = reload; p0->crouching = crouch;
    p0->in_jump = jump; 
    p0->in_ability = ability;
    was_holding_jump = jump;
    
    for (int hi = 0; hi < MAX_HELICOPTERS; hi++) {
        HelicopterState *h = &local_state.helicopters[hi];
        if (!h->active) continue;
        if (h->occupant_player_id >= 0 && h->occupant_player_id < MAX_CLIENTS) {
            PlayerState *occ = &local_state.players[h->occupant_player_id];
            h->input.forward = occ->in_fwd;
            h->input.yaw = occ->in_strafe;
            h->input.strafe = occ->in_ability ? -1.0f : (occ->in_bike ? 1.0f : 0.0f);
            h->input.ascend = occ->in_jump;
            h->input.descend = occ->crouching;
            heli_simulate_step(h, SHANKPIT_NET_FIXED_DT);
            occ->x = h->x; occ->y = h->y; occ->z = h->z;
            occ->yaw = h->yaw; occ->vx = occ->vy = occ->vz = 0.0f;
            occ->on_ground = h->grounded;
        } else {
            h->input.forward = 0.0f; h->input.yaw = 0.0f; h->input.strafe = 0.0f;
            h->input.ascend = 0; h->input.descend = 0;
            heli_simulate_step(h, SHANKPIT_NET_FIXED_DT);
        }
    }

    for(int i=0; i<MAX_CLIENTS; i++) {
        PlayerState *p = &local_state.players[i];
        if (!p->active) continue;
        if (p->in_vehicle && p->vehicle_type == VEH_HELICOPTER) {
            continue;
        }
        if (i > 0 && p->active && p->state != STATE_DEAD) {
            float b_fwd=0, b_yaw=p->yaw;
            int b_btns=0;
            bot_think(i, local_state.players, &b_fwd, &b_yaw, &b_btns);
            p->yaw = b_yaw;
            float brad = b_yaw * 3.14159f / 180.0f;
            float bx = sinf(brad) * b_fwd;
            float bz = cosf(brad) * b_fwd;
            accelerate(p, bx, bz, MAX_SPEED, ACCEL);
            p->in_shoot = (b_btns & BTN_ATTACK);
            p->in_jump = (b_btns & BTN_JUMP);
            p->in_reload = (b_btns & BTN_RELOAD);
            p->crouching = (b_btns & BTN_CROUCH);
            p->in_ability = 0;
            if ((b_btns & BTN_JUMP) && p->on_ground) { p->y += 0.1f; p->vy += JUMP_FORCE; }
        }
        phys_set_scene(p->scene_id);
        update_entity(p, 0.016f, server_context, cmd_time);
    }
    update_projectiles(cmd_time);
    if (local_state.game_mode == MODE_TDMB) {
        for (int i = 0; i < MAX_CLIENTS; i++) {
            PlayerState *pp = &local_state.players[i];
            int prev = tdmb_last_kills[i];
            if (pp->kills > prev && (pp->team_id == TDMB_BLUE_TEAM || pp->team_id == TDMB_RED_TEAM)) {
                int delta = pp->kills - prev;
                local_state.team_scores[pp->team_id] += delta;
                if (!local_state.match_over && local_state.team_scores[pp->team_id] >= local_state.score_limit) {
                    local_state.match_over = 1;
                    local_state.winning_team = pp->team_id;
                }
            }
            tdmb_last_kills[i] = pp->kills;
        }
    }
}

void local_init_match(int num_players, int mode) {
    memset(&local_state, 0, sizeof(ServerState));
    memset(tdmb_last_kills, 0, sizeof(tdmb_last_kills));
    local_state.game_mode = mode;
    scene_set_game_mode(mode);
    local_state.pending_scene = -1;
    local_state.transition_timer = 0;
    local_state.winning_team = -1;
    local_state.score_limit = (mode == MODE_TDMB) ? TDMB_SCORE_LIMIT : 0;

    if (mode == MODE_TDMB) {
        num_players = 1 + TDMB_BLUE_BOTS + TDMB_RED_BOTS;
        local_state.scene_id = SCENE_VOXWORLD;
    } else {
        local_state.scene_id = SCENE_GARAGE_OSAKA;
    }

    phys_set_scene(local_state.scene_id);
    for (int i = 0; i < MAX_CLIENTS; i++) {
        local_state.players[i].id = i;
    }

    local_state.players[0].active = 1;
    local_state.players[0].is_bot = 0;
    local_state.players[0].team_id = (mode == MODE_TDMB) ? TDMB_BLUE_TEAM : ((mode == MODE_TDM || mode == MODE_CTF) ? 0 : -1);
    local_state.players[0].scene_id = local_state.scene_id;
    phys_respawn(&local_state.players[0], 0);

    for(int i=1; i<num_players; i++) {
        local_state.players[i].active = 1;
        local_state.players[i].is_bot = 1;
        if (mode == MODE_TDMB) {
            local_state.players[i].team_id = (i <= TDMB_BLUE_BOTS) ? TDMB_BLUE_TEAM : TDMB_RED_TEAM;
        } else {
            local_state.players[i].team_id = (mode == MODE_TDM || mode == MODE_CTF) ? (i % 2) : -1;
        }
        local_state.players[i].scene_id = local_state.scene_id;
        phys_respawn(&local_state.players[i], i*100);
        init_genome(&local_state.players[i].brain);
    }
    scene_load(local_state.scene_id);
}

#endif
