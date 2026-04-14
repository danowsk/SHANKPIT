#ifndef LOCAL_GAME_H
#define LOCAL_GAME_H

#include "../common/protocol.h"
#include "../common/physics.h"
#include "../common/shared_movement.h"
#include <string.h>

ServerState local_state;
int was_holding_jump = 0;
#define SHANKPIT_HELI_DEBUG 0
#define STICKY_MAX_PER_PLAYER 4
#define STICKY_FUSE_TICKS 105
#define STICKY_THROW_COOLDOWN 24

static inline void pickup_spawn(int scene_id, int type, float x, float y, float z, float radius, int respawn_delay_ticks, int dropped_by_player_id) {
    for (int i = 0; i < MAX_WORLD_PICKUPS; i++) {
        WorldPickup *p = &local_state.world_pickups[i];
        if (p->active) continue;
        memset(p, 0, sizeof(*p));
        p->active = 1;
        p->available = 1;
        p->id = i;
        p->scene_id = scene_id;
        p->type = (unsigned char)type;
        p->x = x; p->y = y; p->z = z;
        p->radius = radius;
        p->respawn_delay_ticks = respawn_delay_ticks;
        p->dropped_by_player_id = dropped_by_player_id;
        return;
    }
}

static inline void poo_poo_island_init_pickups(void) {
    pickup_spawn(SCENE_POO_POO_ISLAND, PICKUP_STICKY_GRENADE, -560.0f, 20.0f, -560.0f, 5.0f, 600, -1);
    pickup_spawn(SCENE_POO_POO_ISLAND, PICKUP_STICKY_GRENADE, 90.0f, 18.0f, -330.0f, 5.0f, 600, -1);
    pickup_spawn(SCENE_POO_POO_ISLAND, PICKUP_STICKY_GRENADE, 250.0f, 45.0f, 210.0f, 5.0f, 600, -1);
    pickup_spawn(SCENE_POO_POO_ISLAND, PICKUP_HEALTH, -510.0f, 15.0f, -660.0f, 5.0f, 450, -1);
    pickup_spawn(SCENE_POO_POO_ISLAND, PICKUP_HEALTH, -300.0f, 16.0f, -420.0f, 5.0f, 450, -1);
    pickup_spawn(SCENE_POO_POO_ISLAND, PICKUP_HEALTH, 420.0f, 52.0f, -120.0f, 5.0f, 450, -1);
}

static inline void sticky_spawn_from_player(PlayerState *p) {
    if (!p || p->sticky_grenades <= 0 || p->sticky_throw_cooldown > 0) return;
    for (int i = 0; i < MAX_STICKY_GRENADES; i++) {
        StickyGrenadeState *g = &local_state.sticky_grenades[i];
        if (g->active) continue;
        memset(g, 0, sizeof(*g));
        g->active = 1;
        g->id = i;
        g->scene_id = p->scene_id;
        g->owner_player_id = p->id;
        g->x = p->x;
        g->y = p->y + EYE_HEIGHT;
        g->z = p->z;
        float r = -p->yaw * 0.0174533f, rp = p->pitch * 0.0174533f;
        float speed = 3.2f;
        g->vx = sinf(r) * cosf(rp) * speed;
        g->vy = sinf(rp) * speed + 0.15f;
        g->vz = -cosf(r) * cosf(rp) * speed;
        g->fuse_ticks = STICKY_FUSE_TICKS;
        p->sticky_grenades--;
        p->sticky_throw_cooldown = STICKY_THROW_COOLDOWN;
        return;
    }
}

static inline void drop_player_inventory_pickups(PlayerState *p) {
    if (!p || p->sticky_grenades <= 0) return;
    phys_set_scene(p->scene_id);
    int src_terrain = 0;
    float gy = phys_sample_ground_height(p->x, p->z, &src_terrain);
    pickup_spawn(p->scene_id, PICKUP_STICKY_GRENADE, p->x, gy + 1.0f, p->z, 5.0f, 0, p->id);
    p->sticky_grenades--;
}

static inline void sticky_explode(StickyGrenadeState *g) {
    if (!g || !g->active) return;
    for (int i = 0; i < MAX_CLIENTS; i++) {
        PlayerState *t = &local_state.players[i];
        if (!t->active || t->state == STATE_DEAD || t->scene_id != g->scene_id) continue;
        float dx = t->x - g->x;
        float dy = (t->y + 2.0f) - g->y;
        float dz = t->z - g->z;
        float d2 = dx * dx + dy * dy + dz * dz;
        if (d2 > 100.0f) continue;
        float d = sqrtf(d2);
        float f = 1.0f - (d / 10.0f);
        int dmg = (int)(120.0f * f);
        if (dmg < 1) dmg = 1;
        t->shield_regen_timer = SHIELD_REGEN_DELAY;
        if (t->shield > 0) {
            if (t->shield >= dmg) { t->shield -= dmg; dmg = 0; }
            else { dmg -= t->shield; t->shield = 0; }
        }
        t->health -= dmg;
        float inv = (d > 0.01f) ? (1.0f / d) : 0.0f;
        float pop = (g->attach_type == STICKY_ATTACH_PLAYER && g->attach_target_id == i) ? 5.8f : 2.2f;
        t->vx += dx * inv * pop;
        t->vz += dz * inv * pop;
        t->vy += (g->attach_type == STICKY_ATTACH_PLAYER && g->attach_target_id == i) ? 3.8f : 1.2f;
        if (t->health <= 0) {
            if (g->owner_player_id >= 0 && g->owner_player_id < MAX_CLIENTS) local_state.players[g->owner_player_id].kills++;
            t->deaths++;
            drop_player_inventory_pickups(t);
            phys_respawn(t, 0);
        }
    }
    g->exploded = 1;
    g->active = 0;
}

static inline void sticky_update_all(void) {
    for (int i = 0; i < MAX_STICKY_GRENADES; i++) {
        StickyGrenadeState *g = &local_state.sticky_grenades[i];
        if (!g->active) continue;
        if (g->fuse_ticks > 0) g->fuse_ticks--;
        if (g->attached && g->attach_type == STICKY_ATTACH_PLAYER && g->attach_target_id >= 0 && g->attach_target_id < MAX_CLIENTS) {
            PlayerState *t = &local_state.players[g->attach_target_id];
            if (t->active && t->state != STATE_DEAD && t->scene_id == g->scene_id) {
                g->x = t->x + g->attach_local_x;
                g->y = t->y + g->attach_local_y;
                g->z = t->z + g->attach_local_z;
            } else {
                g->attach_type = STICKY_ATTACH_WORLD;
            }
        } else if (!g->attached) {
            float nx = 0, ny = 0, nz = 0, hx = 0, hy = 0, hz = 0;
            float nxp = g->x + g->vx, nyp = g->y + g->vy, nzp = g->z + g->vz;
            if (trace_map(g->x, g->y, g->z, nxp, nyp, nzp, &hx, &hy, &hz, &nx, &ny, &nz)) {
                g->attached = 1;
                g->attach_type = STICKY_ATTACH_WORLD;
                g->x = hx; g->y = hy; g->z = hz;
                g->normal_x = nx; g->normal_y = ny; g->normal_z = nz;
                g->vx = g->vy = g->vz = 0.0f;
            } else {
                g->x = nxp; g->y = nyp; g->z = nzp;
                for (int t = 0; t < MAX_CLIENTS; t++) {
                    PlayerState *pl = &local_state.players[t];
                    if (!pl->active || pl->state == STATE_DEAD || pl->scene_id != g->scene_id) continue;
                    if (t == g->owner_player_id) continue;
                    float dx = pl->x - g->x, dy = (pl->y + 2.0f) - g->y, dz = pl->z - g->z;
                    if ((dx*dx + dy*dy + dz*dz) < 8.0f) {
                        g->attached = 1;
                        g->attach_type = STICKY_ATTACH_PLAYER;
                        g->attach_target_id = t;
                        g->attach_local_x = g->x - pl->x;
                        g->attach_local_y = g->y - pl->y;
                        g->attach_local_z = g->z - pl->z;
                        g->vx = g->vy = g->vz = 0.0f;
                        break;
                    }
                }
            }
        }
        if (g->fuse_ticks <= 0) sticky_explode(g);
    }
}

static inline void pickup_update_and_collect(void) {
    for (int i = 0; i < MAX_WORLD_PICKUPS; i++) {
        WorldPickup *wp = &local_state.world_pickups[i];
        if (!wp->active) continue;
        if (!wp->available) {
            if (wp->respawn_delay_ticks > 0 && ++wp->respawn_ticks >= wp->respawn_delay_ticks) {
                wp->respawn_ticks = 0;
                wp->available = 1;
            }
            continue;
        }
        for (int j = 0; j < MAX_CLIENTS; j++) {
            PlayerState *p = &local_state.players[j];
            if (!p->active || p->state == STATE_DEAD || p->scene_id != wp->scene_id) continue;
            float dx = p->x - wp->x, dy = p->y - wp->y, dz = p->z - wp->z;
            if ((dx*dx + dy*dy + dz*dz) > (wp->radius * wp->radius)) continue;
            if (wp->type == PICKUP_HEALTH) {
                if (p->health >= 100) continue;
                p->health += 35; if (p->health > 100) p->health = 100;
            } else if (wp->type == PICKUP_STICKY_GRENADE) {
                if (p->sticky_grenades >= STICKY_MAX_PER_PLAYER) continue;
                p->sticky_grenades++;
            }
            if (wp->respawn_delay_ticks > 0) wp->available = 0;
            else wp->active = 0;
            break;
        }
    }
}

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
    for (int gi = 0; gi < MAX_STICKY_GRENADES; gi++) {
        memset(&local_state.sticky_grenades[gi], 0, sizeof(local_state.sticky_grenades[gi]));
        local_state.sticky_grenades[gi].id = gi;
    }
    for (int pi = 0; pi < MAX_WORLD_PICKUPS; pi++) {
        memset(&local_state.world_pickups[pi], 0, sizeof(local_state.world_pickups[pi]));
        local_state.world_pickups[pi].id = pi;
    }
    if (scene_id == SCENE_POO_POO_ISLAND) {
        poo_poo_island_init_pickups();
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
    if (me->state == STATE_DEAD) { *out_buttons = 0; return; }

    int target_idx = -1;
    float min_dist = 9999.0f;

    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (i == bot_idx) continue;
        if (!players[i].active) continue;
        if (players[i].state == STATE_DEAD) continue;
        
        float dx = players[i].x - me->x;
        float dz = players[i].z - me->z;
        float dist = sqrtf(dx*dx + dz*dz);
        
        if (i == 0 || dist < min_dist) { 
            if (i == 0) dist *= 0.5f;
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
    target->shield_regen_timer = SHIELD_REGEN_DELAY;
    if (target->shield > 0) {
        if (target->shield >= damage) { target->shield -= damage; damage = 0; }
        else { damage -= target->shield; target->shield = 0; }
    }
    target->health -= damage;
    if (target->health <= 0) {
        if (owner) { owner->kills++; owner->accumulated_reward += 500.0f; }
        target->deaths++;
        drop_player_inventory_pickups(target);
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
        if (p->sticky_throw_cooldown > 0) p->sticky_throw_cooldown--;
        if (p->in_grenade && p->state != STATE_DEAD) {
            sticky_spawn_from_player(p);
        }
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
    sticky_update_all();
    pickup_update_and_collect();
}

void local_init_match(int num_players, int mode) {
    memset(&local_state, 0, sizeof(ServerState));
    local_state.game_mode = mode;
    scene_set_game_mode(mode);
    local_state.scene_id = SCENE_GARAGE_OSAKA;
    local_state.pending_scene = -1;
    local_state.transition_timer = 0;
    phys_set_scene(local_state.scene_id);
    local_state.players[0].active = 1;
    local_state.players[0].team_id = 0;
    local_state.players[0].scene_id = local_state.scene_id;
    phys_respawn(&local_state.players[0], 0);
    for(int i=1; i<num_players; i++) {
        local_state.players[i].active = 1;
        local_state.players[i].team_id = (mode == MODE_TDM || mode == MODE_CTF) ? (i % 2) : -1;
        local_state.players[i].scene_id = local_state.scene_id;
        phys_respawn(&local_state.players[i], i*100);
        init_genome(&local_state.players[i].brain);
    }
    scene_load(local_state.scene_id);
}
#endif
