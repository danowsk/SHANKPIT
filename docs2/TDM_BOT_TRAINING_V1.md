# SHANKPIT TDM Bot Training Infrastructure v1

This pass unifies runtime bot policy selection and adds local train/eval/record plumbing without adding in-engine RL.

## Modes

### TRAIN_TDM_LOCAL
Run local server with autonomous bot-v-bot TDM:

```bash
make server
./bin/shank_server --train-tdm-local
```

### EVAL_TDM_LOCAL
Run deterministic local eval match and print summary metrics when match ends:

```bash
./bin/shank_server --eval-tdm-local
```

### RECORD_TDM
Enable JSONL step recording for offline Python/Colab training:

```bash
./bin/shank_server --train-tdm-local --record-tdm --record-tdm-file tdm_records.jsonl
```

Default output file (if no path provided): `tdm_records.jsonl` in current working directory.

## Bot policy selection

Canonical policy runtime lives in `packages/simulation/bot_policy.h/.c`.

Policy order:
1. `BOT_POLICY_NEURAL` if compiled weights are available via `brain_weights.h`.
2. `BOT_POLICY_SCRIPTED` deterministic baseline fallback.
3. `BOT_POLICY_GENOME` is kept for explicit legacy compatibility only.

If neural weights are not available/invalid, runtime logs fallback and uses scripted baseline.

## Observation + action schema

Observation schema is centralized in `BotObservation` and packed via `pack_bot_observation_vec()` to preserve 8-float NN compatibility.

Action schema is centralized in `BotAction` and mapped to usercmd fields by `bot_policy_action_to_usercmd()`.

## Recording format

`RECORD_TDM` writes JSONL lines with:
- tick, match_id, bot_id, team_id
- observation vector
- action fields
- reward breakdown components
- done flag and health

This is designed for direct ingestion from Python/Colab.

## Weights plug-in point

Neural runtime still uses compiled weights from:

- `packages/simulation/brain_weights.h`

Keep exporting updated weights to that header for runtime inference.
