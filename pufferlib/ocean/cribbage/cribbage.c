/* Build it with:
 * bash scripts/build_ocean.sh cribbage local (debug)
 * bash scripts/build_ocean.sh cribbage fast
 * We suggest building and debugging your env in pure C first. You
 * get faster builds and better error messages
 */
#include "cribbage.h"

/* Puffernet is our lightweight cpu inference library that
 * lets you load basic PyTorch model architectures so that
 * you can run them in pure C or on the web via WASM
 */
// #include "puffernet.h"

const int NUM_PLAYERS = 2;
const int OBSERVATION_SIZE = 18;  // hand (6), cut card (7), played cards (15), running total (16),
                                  // player score (17), opponent score (18)
// last card is forced so maybe I don't need to include it in obs? Probably easier to just inclued
// it.

int main() {
  // Weights are exported by running puffer export
  // Weights* weights = load_weights("resources/target/target_weights.bin", 137743);

  // int logit_sizes[2] = {9, 5};
  // LinearLSTM* net = make_linearlstm(weights, num_agents, num_obs, logit_sizes, 2);

  Cribbage env;
  init(&env);  // TODO: does this work without this?

  // Allocate these manually since they aren't being passed from Python
  env.observations = calloc(NUM_PLAYERS * OBSERVATION_SIZE, sizeof(int));
  env.actions = calloc(2, sizeof(int));  // 1-16 for crib, 1-4 for play
  env.rewards = calloc(NUM_PLAYERS, sizeof(float));
  env.terminals = calloc(NUM_PLAYERS, sizeof(unsigned char));

  // Always call reset and render first
  c_reset(&env);
  c_render(&env);
  reset_lasttwo(&env);

  // while(True) will break web builds
  while (!WindowShouldClose()) {
    if (env.phase == 0) {
      env.actions[1] = rand() % 15;
    } else {
      env.actions[1] = rand() % 4;
    }

    if (IsKeyPressed(KEY_ONE)) lasttwo(&env, 0);
    if (IsKeyPressed(KEY_TWO)) lasttwo(&env, 1);
    if (IsKeyPressed(KEY_THREE)) lasttwo(&env, 2);
    if (IsKeyPressed(KEY_FOUR)) lasttwo(&env, 3);
    if (IsKeyPressed(KEY_FIVE)) lasttwo(&env, 4);
    if (IsKeyPressed(KEY_SIX)) lasttwo(&env, 5);
    if (IsKeyPressed(KEY_SEVEN)) lasttwo(&env, 6);
    if (IsKeyPressed(KEY_ENTER)) {
      if (env.phase == 0) {
        env.actions[0] = lasttwo_crib(&env);
        if (env.actions[0] != -1) {
          c_step(&env);
          reset_lasttwo(&env);
        }
      } else if (env.client->lastone != -1) {
        env.actions[0] = env.client->lastone;
        c_step(&env);
        reset_lasttwo(&env);
      }
    }

    // forward_linearlstm(net, env.observations, env.actions);
    c_render(&env);
  }

  // Try to clean up after yourself
  // free_linearlstm(net);
  free(env.observations);
  free(env.actions);
  free(env.rewards);
  free(env.terminals);
  c_close(&env);
}
