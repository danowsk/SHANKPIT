# SHANKPIT TDM Bot Training Infrastructure (v1)

This pass unifies runtime bot policy selection and adds local train/eval/record plumbing without implementing in-engine learning.

## Runtime policy selection

- Canonical policy runtime is in `packages/simulation/bot_policy.h`.
- Policy kinds:
  - `BOT_POLICY_SCRIPTED` (deterministic fallback baseline)
  - `BOT_POLICY_GENOME` (legacy compatibility path)
  - `BOT_POLICY_NEURAL` (current exported-weight inference path)
- Default for TDM family modes is:
  - neural if compiled `brain_weights.h` shape is valid
  - otherwise deterministic scripted baseline

## Observation/action schema

- Observation source of truth: `BotObservation` + `build_bot_observation(...)`.
- Neural compatibility packer: `pack_bot_observation_vec(...)` (8 named features).
- Action source of truth: `BotAction`.
- Mapping from policy output into game controls is centralized by `bot_action_to_usercmd(...)`.

## Local harness flags

Harness is controlled by env vars (server-first friendly):

- `SHANKPIT_TDM_HARNESS=TRAIN_TDM_LOCAL`
- `SHANKPIT_TDM_HARNESS=EVAL_TDM_LOCAL`
- `SHANKPIT_RECORD_TDM=1`
- `SHANKPIT_RECORD_TDM_PATH=...` (optional, default `tdm_recording.jsonl`)
- `SHANKPIT_EVAL_TICKS=6000` (optional)

### TRAIN_TDM_LOCAL

Runs local deterministic TDMB map setup with all bots and unified bot policy interface.

### EVAL_TDM_LOCAL

Runs deterministic local TDMB eval setup and prints per-match + per-bot summary lines.

### RECORD_TDM

When enabled, writes JSONL records for:
- per-step transitions (`event=step`)
- per-match summaries (`event=match_summary`)

Each step record includes tick, match id, bot id, team id, observation, action, reward components, done/death flags.

## Weights plug-in point

Neural inference still uses compiled `packages/simulation/brain_weights.h` through `packages/simulation/neural_net.h`.

Future export pass should continue generating `brain_weights.h` with the same architecture unless a compatibility shim is updated.
