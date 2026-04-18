#include "../../packages/simulation/local_game.h"
#include <stdlib.h>

// Export functions for Python
void sim_init(int bots) {
    local_init_match(bots, 0);
}

void sim_init_train_tdm_local(void) {
    setenv("SHANKPIT_TDM_HARNESS", "TRAIN_TDM_LOCAL", 1);
    local_init_match(12, MODE_TDMB);
}

void sim_init_eval_tdm_local(void) {
    setenv("SHANKPIT_TDM_HARNESS", "EVAL_TDM_LOCAL", 1);
    local_init_match(12, MODE_TDMB);
}

void sim_enable_tdm_recording(const char *path) {
    setenv("SHANKPIT_RECORD_TDM", "1", 1);
    if (path && path[0]) setenv("SHANKPIT_RECORD_TDM_PATH", path, 1);
}

void sim_step(float fwd, float strafe, float yaw, float pitch, int shoot, int jump) {
    // We are controlling Player 0 (The Agent)
    // The "bots" in the match are the opponents
    local_update(fwd, strafe, yaw, pitch, shoot, -1, jump, 0, 0, 0, NULL, 0);
}

ServerState* sim_get_state() {
    return &local_state;
}
