#include <stdio.h>
#include <netinet/in.h>

#include "../../packages/common/net_sim.h"
#include "../../packages/simulation/local_game.h"

static int tests_run = 0;
static int tests_passed = 0;

#define ASSERT_TRUE(cond, msg) do { \
    tests_run++; \
    if (!(cond)) { \
        printf("❌ %s\n", msg); \
    } else { \
        printf("✅ %s\n", msg); \
        tests_passed++; \
    } \
} while (0)

int main(void) {
    printf("--- CTFB Carry Melee Regression ---\n");
    local_init_match(12, MODE_CTFB);

    PlayerState *attacker = &local_state.players[0];
    PlayerState *target = &local_state.players[7];
    unsigned int now_ms = 1000;

    attacker->state = STATE_ALIVE;
    attacker->x = 0.0f; attacker->y = 0.0f; attacker->z = 0.0f;
    attacker->yaw = 0.0f; attacker->pitch = 0.0f;
    attacker->carried_flag_team_id = TDMB_RED_TEAM;
    attacker->ctf_melee_cooldown_ms = 0;

    target->state = STATE_ALIVE;
    target->shield = 0;
    target->health = 100;
    target->x = 0.0f; target->y = 0.0f; target->z = -8.0f;
    target->scene_id = attacker->scene_id;
    target->team_id = TDMB_RED_TEAM;

    ctf_try_carry_melee(attacker, now_ms);
    ASSERT_TRUE(target->health == 100 - CTFB_CARRY_MELEE_DAMAGE, "Flag carry melee reuses knife envelope and lands in front arc");
    ASSERT_TRUE(attacker->ctf_melee_cooldown_ms == now_ms + CTFB_CARRY_MELEE_COOLDOWN_MS, "Flag carry melee applies CTF cooldown source of truth");
    ASSERT_TRUE(attacker->is_shooting == 3, "Flag carry melee marks melee fire animation state");

    int health_after_first = target->health;
    ctf_try_carry_melee(attacker, now_ms + 100);
    ASSERT_TRUE(target->health == health_after_first, "Flag carry melee respects cooldown and does not multi-hit early");

    target->health = 100;
    target->x = 8.0f; target->z = 0.0f;
    attacker->ctf_melee_cooldown_ms = 0;
    ctf_try_carry_melee(attacker, now_ms + 900);
    ASSERT_TRUE(target->health == 100, "Flag carry melee misses targets outside knife-style trace envelope");

    printf("SUMMARY: %d/%d Tests Passed.\n", tests_passed, tests_run);
    return (tests_passed == tests_run) ? 0 : 1;
}
