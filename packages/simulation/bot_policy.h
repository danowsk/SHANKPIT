#ifndef BOT_POLICY_H
#define BOT_POLICY_H

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#endif

#include "../common/protocol.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    BOT_POLICY_SCRIPTED = 0,
    BOT_POLICY_GENOME = 1,
    BOT_POLICY_NEURAL = 2
} BotPolicyKind;

typedef struct {
    float rel_enemy_x;
    float rel_enemy_z;
    float enemy_distance_norm;
    float aim_error_norm;
    float self_health_norm;
    float ammo_pressure;
    float team_crowding;
    float danger_signal;
} BotObservation;

typedef struct {
    float forward;
    float strafe;
    float yaw_rate;
    int shoot;
    int jump;
    int crouch;
    int reload;
} BotAction;

typedef struct {
    float damage_dealt;
    float kill;
    float death;
    float self_damage;
    float time_alive;
    float aim_on_target;
    float idle_penalty;
    float stuck_penalty;
    float team_score_delta;
    float total;
} BotRewardBreakdown;

typedef struct {
    int strafe_phase;
    float prev_x;
    float prev_z;
    int had_prev_pos;
    int stuck_ticks;
} BotPolicyRuntime;

int bot_policy_neural_weights_available(void);
BotPolicyKind bot_policy_select_default(int allow_genome_fallback);

int bot_policy_find_target(int bot_idx, const ServerState *world, int require_los);
void build_bot_observation(int bot_idx, const ServerState *world, int target_idx, BotObservation *obs);
void pack_bot_observation_vec(const BotObservation *obs, float vec8[8]);

void bot_policy_scripted_eval(int bot_idx, const ServerState *world, int target_idx, const BotObservation *obs, BotPolicyRuntime *rt, BotAction *action);
void bot_policy_genome_eval(int bot_idx, const ServerState *world, int target_idx, BotAction *action);
void bot_policy_neural_eval(const BotObservation *obs, BotAction *action);

void bot_policy_action_to_usercmd(const BotAction *action, float *out_fwd, float *out_strafe, float *out_yaw, int *out_buttons);

#ifdef __cplusplus
}
#endif

#endif
