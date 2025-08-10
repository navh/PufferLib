/* Cribbage: a two-player cribbage game.
 * Star PufferLib on GitHub to support. It really, really helps!
 *
 * Rules of this variant of Cribbage:
 * - No muggins, no skunks, cuts and such are replaced with random number generators.
 * - First player to 121 points wins.
 * - Cards are ranked: K (high), Q, J, 10, 9, 8, 7, 6, 5, 4, 3, 2, A.
 * - Coinflip assigns players to be "dealer" or "pone".
 * - Each player is dealt 6 cards. (Note, 52c6 is 20,358,520 ~=2^25) (we need 12 cards)
 * - Each player lays 2 cards in the crib.
 * - PLAY BEGINS
 * - "Cut card" is randomly drawn from remaining deck (we need 13 cards)
 * - If the card is a Jack, dealer scores 2 points.
 * - Pone goes first, then dealer and pone alternate playing cards one at a time.
 * - A running total is kept of the cards played.
 * - Cards 2-10 count as their pip value (numerical value), face cards (J, Q, K) count as 10, and
 * Aces count as 1.
 * - If a player cannot play without exceeding 31, they say "go" and the opponent scores +1 point.
 * - If a player reaches exactly 31, they score +2 points.
 * - After a 31 is played, the opponent starts the next round with the total resetting to zero.
 * - After a "Go," the opponent leads the next round, starting from zero.
 * - The player who plays the last card gets +1 point for "Go" and +2 points for exactly 31.
 * - Fifteen: If the total reaches 15, +2 points.
 * - Pair: If a card that matches the last card is played, +2 points.
 * - Trip: If you play a third card of the same rank, +6 points.
 * - Quad: If you play a fourth card of the same rank, +12 points.
 * - Run: 3-7 cards in a sequence, +3 points for 3, +4 for 4, +5 for 5, +6 for 6, +7 for 7.
 * - COUNT BEGINS
 * - Pone counts first, then dealer, then crib is counted with points going to the dealer.
 * - Hands are 5 cards, 4 in the hand and the 1 cut card.
 * - Fifteen: Any combination of cards that adds up to 15. +2 points for each combination.
 * - Pair: Two cards of the same rank. +2 points for each pair.
 * - Run: 3-5 cards in a sequence, +3 points for 3, +4 for 4, +5 for 5.
 * - Flush: 4 cards of the same suit, +4 points. +5 for 5 of the same suit.
 * - Jackmatch: Jack of the same suit as the cut card, +1 point.
 * - (note: test that cut5, Jmatch 5,5,5 scores perfect 29)
 * - Dealer and pone alternate, new cards are delt, and game repeats until 121.
 *
 * -----------------------------------------------------------------------------
 * Arbitrary decisions:
 * - cards are indexed, I hope to fit this in 8 bits, also to be useful for other card games.
 * - 0 is reserved for "unknown", maybe I should distinguish between "no card" and "unknown"
 * - 1-52 are A,2,3,4,5,6,7,8,9,10,J,Q,K for club, diamond, heart, spade.
 *
 * - step rewards should be 0-1, so I'll divide by 29.
 * - maybe I should just be doing a 1/0 win/nowin on episodes though?
 * - in reality I think it should be some sort of ELO-like rating system?
 * - this is me thinking ahead to my 'fish finder' project.
 *
 * - I asked about self-play, jsuarez said to just do marl and ignore non-player actions
 * - disgusting. away we go.
 *
 * - Cards are 13x19
 * - 9 cards wide with 1px between each, 9x13+10=127px wide
 * - too small, maybe 5x cards on both dimensions?
 * - this allows a 640x480 window
 */

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "raylib.h"

const int CRIB_LUT[15][2] = {
    {0, 1},  // 0
    {0, 2},  // 1
    {0, 3},  // 2
    {0, 4},  // 3
    {0, 5},  // 4
    {1, 2},  // 5
    {1, 3},  // 6
    {1, 4},  // 7
    {1, 5},  // 8
    {2, 3},  // 9
    {2, 4},  // 10
    {2, 5},  // 11
    {3, 4},  // 12
    {3, 5},  // 13
    {4, 5},  // 14
};

const int LASTTWO_LUT[6][6] = {
    {-1, 0, 1, 2, 3, 4},     // 00, 01, 02, 03, 04, 05
    {0, -1, 5, 6, 7, 8},     // 10, 11, 12, 13, 14, 15
    {1, 5, -1, 9, 10, 11},   // 20, 21, 22, 23, 24, 25
    {2, 6, 9, -1, 12, 13},   // 30, 31, 32, 33, 34, 35
    {3, 7, 10, 12, -1, 14},  // 40, 41, 42, 43, 44, 45
    {4, 8, 11, 13, 14, -1},  // 50, 51, 52, 53, 54, 55
};

const int CARD_VALUES[53] = {
    0, 
    1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 10, 10, 10, 
    1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 10, 10, 10,
    1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 10, 10, 10, 
    1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 10, 10, 10,
};

const int CARD_RANKS[53] = {
    0, 
    1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 
    1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13,
    1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 
    1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13,
};

const int CARD_SUITS[53] = {
    0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
    3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
};

typedef struct {
  float perf;            // Recommended 0-1 normalized single real number perf metric
  float score;           // Recommended unnormalized single real number perf metric
  float episode_return;  // Recommended metric: sum of agent rewards over episode
  float episode_length;  // Recommended metric: number of steps of agent episode
  // Any extra fields you add here may be exported to Python in binding.c
  float n;  // Required as the last field
} Log;

typedef struct {
  Texture2D card_back;
  Texture2D cards[53];
  int lasttwo[2];
  int lastone;
} Client;

typedef struct {
  Log log;  // Required field. Env binding code uses this to aggregate logs
  Client* client;
  int* observations;  // Required. You can use any obs type, but make sure it matches in Python! //
                      // TODO: can I use unsigned char (or some int8?) in python?
  int* actions;       // Required. int* for discrete/multidiscrete, float* for box
  float* rewards;     // Required
  unsigned char* terminals;  // Required. We don't yet have truncations as standard yet
  uint_fast8_t dealer;       // 0 or 1
  uint_fast8_t hand[12];     // [0-6] p0 hand, [7-11] p1 hand
  uint_fast8_t crib[4];      // [0-1] p0 discards, [2-3] p1 discards
  uint_fast8_t cut_card;
  uint_fast8_t score[2];  // [0] p0, [1] p1
  uint_fast8_t hand_score[2];
  uint_fast8_t played_cards[8];
  uint_fast8_t count;
  uint_fast8_t phase;        // 0: crib, 1-8: play,
  uint_fast8_t player_turn;  // 0 for p0, 1 for p1
} Cribbage;

/* Recommended to have an init function of some kind if you allocate
 * extra memory. This should be freed by c_close. Don't forget to call
 * this in binding.c!
 */
void init(Cribbage* env) {
  // TODO: I think I can just scrap init?
  //
  //
  env->client = NULL;  // HACK: I don't know why I needed to do this.
                       // It seems like on all the other envs, the "client==NULL" check works,
                       // and I don't see it being explicitly set anywhere.
                       // however, I kept getting non-null and this fixed it.
}

/* Recommended to have an observation function of some kind because
 * you need to compute agent observations in both reset and in step.
 */
void compute_observations(Cribbage* env) {
  //    const int OBSERVATION_SIZE = 16; // 6 card hand, 8 cards, turn card, running total, scores

  // TODO: hand, cut card, cards played, running total, player score, opponent score
  int obs_idx = 0;
  for (int player = 0; player < 2; player++) {
    // construct observation
    for (int i = 0; i < 4; i++) {
      env->observations[player * 4 + i] = env->hand[player * 4 + i];
    }
    for (int i = 0; i < 8; i++) {
      env->observations[obs_idx++] = env->played_cards[player * 8 + i];
    }
    env->observations[obs_idx++] = env->cut_card;
    env->observations[obs_idx++] = env->count;
  }
}

void deal_cards(Cribbage* env) {
  memset(env->played_cards, 0, sizeof(env->played_cards));
  memset(env->crib, 0, sizeof(env->crib));
  env->hand_score[0] = 0;
  env->hand_score[1] = 0;
  env->count = 0;
  env->phase = 0;
  env->player_turn = (env->dealer + 1) % 2;

  int deck[52];
  for (int i = 0; i < 52; i++) {
    deck[i] = i + 1;
  }
  for (int i = 0; i < 13; i++) {
    int random_location = i + rand() % (52 - i);
    printf("random_location: %d\n", random_location);
    deck[i] = deck[random_location];
    deck[random_location] = i + 1;  // swap relies on deck starting in order
  }
  for (int i = 0; i < 12; i++) {
    env->hand[i] = deck[i];
  }
  env->cut_card = deck[12];
  memset(env->crib, 0, sizeof(env->crib));
}

// Required function
void c_reset(Cribbage* env) {
  env->dealer = rand() % 2;
  env->score[0] = 0;
  env->score[1] = 0;
  deal_cards(env);
  compute_observations(env);
}

void play_card(Cribbage* env, int card_idx) {
  if (env->count + CARD_VALUES[card_idx] <= 31) {
    env->count += CARD_VALUES[card_idx];
    env->played_cards[env->phase - 1] = card_idx;
    env->phase++;
    env->hand[card_idx] = 0;
  }
  if (env->count == 31){
    env->score[env->player_turn] += 2;
  }
  env->player_turn = (env->player_turn + 1) % 2;
}

int play_phase(Cribbage* env) {
  //
  // if current player is forced or "go", or
  // if not, assume they've just made a decision and unpack their action
  //
  // add card to played cards
  // increment running total
  // check for 15, pair, run, flush
  // check for 31
  // hand control off to the other player
  // hand control back to the first player if they're forced.
  // keep incrementing phase until we're done.
  // if a player needs to unpack an action, return 0

  // TODO: pick up here,
  // Need to enable the actual play phase to take place.

  // Once both players are out of cards

  // if (env->phase == 9) {
  //   // score the hands, and start over
  //   env->score[((env->dealer + 1) % 2)] += env->hand_score[(env->dealer + 1) % 2];
  //   if (env->score[((env->dealer + 1) % 2)] >= 121) {
  //     printf("Player %d wins!\n", ((env->dealer + 1) % 2));
  //   }
  //   env->score[env->dealer] += env->hand_score[env->dealer];
  //   if (env->score[env->dealer] >= 121) {
  //     printf("Player %d wins!\n", env->dealer);
  //   }
  //   deal_cards(env);
  //   env->phase = 0;
  //   env->player_turn = (env->dealer + 1) % 2;
  //   return 0;
  // }
  // else if (env->phase == 8) {
  //   // one card left is always forced
  //   int last_card = 0;
  //   for (int i = 0; i < 12; i++) {
  //     if (env->hand[i] != 0) {
  //       last_card = env->hand[i];
  //       break;
  //     }
  //   }
  //   env->played_cards[env->phase - 1] = last_card;
  // }
  // // todo: I think I started this backwards,
  // I think I should instead assume that the player's action is good,
  // and then run down any forced moves after the fact.

  int card_selected = env->hand[env->actions[0] + env->player_turn * 6];
  if (env->count + CARD_VALUES[card_selected] <= 31) {
    env->count += CARD_VALUES[card_selected];
    env->played_cards[env->phase - 1] = card_selected;
    env->phase++;
    env->hand[env->actions[0] + env->player_turn * 6] = 0;
    env->player_turn = (env->player_turn + 1) % 2;
  }

  return 0;  // exit this loop if this works
}

int compare_ints(const void* a, const void* b) { return (*(int*)a - *(int*)b); }

int score_hand(int a, int b, int c, int d, int e) {
  int score = 0;

  // Fifteen: Any combination of cards that adds up to 15. +2 points for each combination.

  if (a + b + c + d + e == 15) score += 2;
  if (a + b + c + d == 15) score += 2;
  if (a + b + c + e == 15) score += 2;
  if (a + b + d + e == 15) score += 2;
  if (a + c + d + e == 15) score += 2;
  if (b + c + d + e == 15) score += 2;
  if (a + b + c == 15) score += 2;
  if (a + b + d == 15) score += 2;
  if (a + b + e == 15) score += 2;
  if (a + c + d == 15) score += 2;
  if (a + c + e == 15) score += 2;
  if (a + d + e == 15) score += 2;
  if (b + c + d == 15) score += 2;
  if (b + c + e == 15) score += 2;
  if (b + d + e == 15) score += 2;
  if (c + d + e == 15) score += 2;
  if (a + b == 15) score += 2;
  if (a + c == 15) score += 2;
  if (a + d == 15) score += 2;
  if (a + e == 15) score += 2;
  if (b + c == 15) score += 2;
  if (b + d == 15) score += 2;
  if (b + e == 15) score += 2;
  if (c + d == 15) score += 2;
  if (c + e == 15) score += 2;
  if (d + e == 15) score += 2;

  // Pair: Two cards of the same rank. +2 points for each pair.

  if (CARD_RANKS[a] == CARD_RANKS[b]) score += 2;
  if (CARD_RANKS[a] == CARD_RANKS[c]) score += 2;
  if (CARD_RANKS[a] == CARD_RANKS[d]) score += 2;
  if (CARD_RANKS[a] == CARD_RANKS[e]) score += 2;
  if (CARD_RANKS[b] == CARD_RANKS[c]) score += 2;
  if (CARD_RANKS[b] == CARD_RANKS[d]) score += 2;
  if (CARD_RANKS[b] == CARD_RANKS[e]) score += 2;
  if (CARD_RANKS[c] == CARD_RANKS[d]) score += 2;
  if (CARD_RANKS[c] == CARD_RANKS[e]) score += 2;
  if (CARD_RANKS[d] == CARD_RANKS[e]) score += 2;

  // Run: 3-5 cards in a sequence, +3 points for 3, +4 for 4, +5 for 5.

  int ranks[5] = {CARD_RANKS[a], CARD_RANKS[b], CARD_RANKS[c], CARD_RANKS[d], CARD_RANKS[e]};
  qsort(ranks, 5, sizeof(int), compare_ints);
  if ((ranks[0] + 1 == ranks[1] && ranks[1] + 1 == ranks[2] && ranks[2] + 1 == ranks[3] &&
       ranks[3] + 1 == ranks[4]) ||
      (ranks[1] + 1 == ranks[2] && ranks[2] + 1 == ranks[3] && ranks[3] + 1 == ranks[4] &&
       ranks[4] == 13 && ranks[0] == 1)) {
    score += 5;
  } else if ((ranks[0] + 1 == ranks[1] && ranks[1] + 1 == ranks[2] && ranks[2] + 1 == ranks[3]) ||
             (ranks[1] + 1 == ranks[2] && ranks[2] + 1 == ranks[3] && ranks[3] + 1 == ranks[4]) ||
             (ranks[2] + 1 == ranks[3] && ranks[3] + 1 == ranks[4] && ranks[4] == 13 &&
              ranks[0] == 1)) {
    score += 4;
  } else if ((ranks[0] + 1 == ranks[1] && ranks[1] + 1 == ranks[2]) ||
             (ranks[1] + 1 == ranks[2] && ranks[2] + 1 == ranks[3]) ||
             (ranks[2] + 1 == ranks[3] && ranks[3] + 1 == ranks[4]) ||
             (ranks[3] + 1 == ranks[4] && ranks[4] == 13 && ranks[0] == 1)) {
    score += 3;
  }

  // Flush: 4 cards of the same suit, +4 points. +5 for 5 of the same suit.
  int suits[4] = {0, 0, 0, 0};
  suits[CARD_SUITS[a]]++;
  suits[CARD_SUITS[b]]++;
  suits[CARD_SUITS[c]]++;
  suits[CARD_SUITS[d]]++;
  suits[CARD_SUITS[e]]++;
  if (suits[CARD_SUITS[1]] >= 4) score += suits[CARD_SUITS[1]];
  if (suits[CARD_SUITS[2]] >= 4) score += suits[CARD_SUITS[2]];
  if (suits[CARD_SUITS[3]] >= 4) score += suits[CARD_SUITS[3]];
  if (suits[CARD_SUITS[4]] >= 4) score += suits[CARD_SUITS[4]];

  // Jackmatch: Jack of the same suit as the cut card, +1 point.

  if (CARD_VALUES[b] == 11 && CARD_SUITS[b] == CARD_SUITS[a]) score += 1;
  if (CARD_VALUES[c] == 11 && CARD_SUITS[c] == CARD_SUITS[a]) score += 1;
  if (CARD_VALUES[d] == 11 && CARD_SUITS[d] == CARD_SUITS[a]) score += 1;
  if (CARD_VALUES[e] == 11 && CARD_SUITS[e] == CARD_SUITS[a]) score += 1;

  // (note: test that cut5, Jmatch 5,5,5 scores perfect 29)
  return score;
}

void crib_phase(Cribbage* env) {
  // crib phase
  // unpack actions
  // copy cards from hands to crib, then zero out the hands
  env->crib[0] = env->hand[CRIB_LUT[env->actions[0]][0]];
  env->crib[1] = env->hand[CRIB_LUT[env->actions[0]][1]];
  env->crib[2] = env->hand[6 + CRIB_LUT[env->actions[1]][0]];
  env->crib[3] = env->hand[6 + CRIB_LUT[env->actions[1]][1]];
  // move all valid cards to the first 4 slots
  env->hand[CRIB_LUT[env->actions[0]][0]] = env->hand[4];
  env->hand[CRIB_LUT[env->actions[0]][1]] = env->hand[5];
  env->hand[6 + CRIB_LUT[env->actions[1]][0]] = env->hand[10];
  env->hand[6 + CRIB_LUT[env->actions[1]][1]] = env->hand[11];
  env->hand[4] = 0;
  env->hand[5] = 0;
  env->hand[10] = 0;
  env->hand[11] = 0;

  // pretend we cut here and assign knob score
  // the jacks are at 11, 11+13 = 24, 11+26 = 37, 11+39 = 50
  if (env->cut_card == 11 || env->cut_card == 24 || env->cut_card == 37 || env->cut_card == 50) {
    env->score[env->dealer] += 2;
  }

  // I think if I do this here I can just throw out the crib
  env->hand_score[0] =
      score_hand(env->cut_card, env->hand[0], env->hand[1], env->hand[2], env->hand[3]);
  env->hand_score[1] =
      score_hand(env->cut_card, env->hand[6], env->hand[7], env->hand[8], env->hand[9]);
  env->hand_score[env->dealer] +=
      score_hand(env->cut_card, env->crib[0], env->crib[1], env->crib[2], env->crib[3]);

  env->count = CARD_VALUES[env->cut_card];

  env->phase = 1;
  env->player_turn = (env->dealer + 1) % 2;
}

// Required function
void c_step(Cribbage* env) {
  if (env->phase == 0) crib_phase(env);
  else play_phase(env);
  compute_observations(env);
}

const int SCALE = 5;

// All of this lasttwo insanity is just to make the client interface work.
void lasttwo(Cribbage* env, int selection) {
  env->client->lastone = selection;
  if (env->client->lasttwo[0] == selection) {
    env->client->lasttwo[0] = -1;
  } else if (env->client->lasttwo[1] == selection) {
    env->client->lasttwo[1] = env->client->lasttwo[0];
    env->client->lasttwo[0] = -1;
  } else if (env->client->lasttwo[1] == -1) {
    env->client->lasttwo[1] = selection;
  } else {
    env->client->lasttwo[0] = env->client->lasttwo[1];
    env->client->lasttwo[1] = selection;
  }
}

int lasttwo_crib(Cribbage* env) {
  if (env->client->lasttwo[0] == -1 || env->client->lasttwo[1] == -1 ||
      env->client->lasttwo[0] == env->client->lasttwo[1]) {
    return -1;
  } else {
    return LASTTWO_LUT[env->client->lasttwo[0]][env->client->lasttwo[1]];
  }
}

void reset_lasttwo(Cribbage* env) {
  env->client->lastone = -1;
  env->client->lasttwo[0] = -1;
  env->client->lasttwo[1] = -1;
}

// Required function. Should handle creating the client on first call
void c_render(Cribbage* env) {
  if (env->client == NULL) {
    env->client = (Client*)calloc(1, sizeof(Client));
    InitWindow(128 * SCALE, 96 * SCALE, "PufferLib Cribbage");
    SetTargetFPS(60);

    // Don't do this before calling InitWindow
    env->client->card_back = LoadTexture("resources/shared/cards/b00.png");
    env->client->cards[0] = LoadTexture("resources/shared/cards/0.png");
    env->client->cards[1] = LoadTexture("resources/shared/cards/1.png");
    env->client->cards[2] = LoadTexture("resources/shared/cards/2.png");
    env->client->cards[3] = LoadTexture("resources/shared/cards/3.png");
    env->client->cards[4] = LoadTexture("resources/shared/cards/4.png");
    env->client->cards[5] = LoadTexture("resources/shared/cards/5.png");
    env->client->cards[6] = LoadTexture("resources/shared/cards/6.png");
    env->client->cards[7] = LoadTexture("resources/shared/cards/7.png");
    env->client->cards[8] = LoadTexture("resources/shared/cards/8.png");
    env->client->cards[9] = LoadTexture("resources/shared/cards/9.png");
    env->client->cards[10] = LoadTexture("resources/shared/cards/10.png");
    env->client->cards[11] = LoadTexture("resources/shared/cards/11.png");
    env->client->cards[12] = LoadTexture("resources/shared/cards/12.png");
    env->client->cards[13] = LoadTexture("resources/shared/cards/13.png");
    env->client->cards[14] = LoadTexture("resources/shared/cards/14.png");
    env->client->cards[15] = LoadTexture("resources/shared/cards/15.png");
    env->client->cards[16] = LoadTexture("resources/shared/cards/16.png");
    env->client->cards[17] = LoadTexture("resources/shared/cards/17.png");
    env->client->cards[18] = LoadTexture("resources/shared/cards/18.png");
    env->client->cards[19] = LoadTexture("resources/shared/cards/19.png");
    env->client->cards[20] = LoadTexture("resources/shared/cards/20.png");
    env->client->cards[21] = LoadTexture("resources/shared/cards/21.png");
    env->client->cards[22] = LoadTexture("resources/shared/cards/22.png");
    env->client->cards[23] = LoadTexture("resources/shared/cards/23.png");
    env->client->cards[24] = LoadTexture("resources/shared/cards/24.png");
    env->client->cards[25] = LoadTexture("resources/shared/cards/25.png");
    env->client->cards[26] = LoadTexture("resources/shared/cards/26.png");
    env->client->cards[27] = LoadTexture("resources/shared/cards/27.png");
    env->client->cards[28] = LoadTexture("resources/shared/cards/28.png");
    env->client->cards[29] = LoadTexture("resources/shared/cards/29.png");
    env->client->cards[30] = LoadTexture("resources/shared/cards/30.png");
    env->client->cards[31] = LoadTexture("resources/shared/cards/31.png");
    env->client->cards[32] = LoadTexture("resources/shared/cards/32.png");
    env->client->cards[33] = LoadTexture("resources/shared/cards/33.png");
    env->client->cards[34] = LoadTexture("resources/shared/cards/34.png");
    env->client->cards[35] = LoadTexture("resources/shared/cards/35.png");
    env->client->cards[36] = LoadTexture("resources/shared/cards/36.png");
    env->client->cards[37] = LoadTexture("resources/shared/cards/37.png");
    env->client->cards[38] = LoadTexture("resources/shared/cards/38.png");
    env->client->cards[39] = LoadTexture("resources/shared/cards/39.png");
    env->client->cards[40] = LoadTexture("resources/shared/cards/40.png");
    env->client->cards[41] = LoadTexture("resources/shared/cards/41.png");
    env->client->cards[42] = LoadTexture("resources/shared/cards/42.png");
    env->client->cards[43] = LoadTexture("resources/shared/cards/43.png");
    env->client->cards[44] = LoadTexture("resources/shared/cards/44.png");
    env->client->cards[45] = LoadTexture("resources/shared/cards/45.png");
    env->client->cards[46] = LoadTexture("resources/shared/cards/46.png");
    env->client->cards[47] = LoadTexture("resources/shared/cards/47.png");
    env->client->cards[48] = LoadTexture("resources/shared/cards/48.png");
    env->client->cards[49] = LoadTexture("resources/shared/cards/49.png");
    env->client->cards[50] = LoadTexture("resources/shared/cards/50.png");
    env->client->cards[51] = LoadTexture("resources/shared/cards/51.png");
    env->client->cards[52] = LoadTexture("resources/shared/cards/52.png");
  }

  // Standard across our envs so exiting is always the same
  if (IsKeyDown(KEY_ESCAPE)) {
    exit(0);
  }

  BeginDrawing();
  ClearBackground((Color){6, 24, 24, 255});

  // draw 6 opponent cards
  for (int i = 0; i < 6; i++) {
    if (env->hand[6 + i] != 0) {
      DrawTextureEx(env->client->card_back, (Vector2){1 * SCALE + 14 * SCALE * i, 11 * SCALE}, 0,
                    SCALE, WHITE);
      // DrawTextureEx(env->client->cards[env->hand[6+i]], (Vector2){1*SCALE+14*SCALE*i, 11*SCALE},
      // 0, SCALE, GREEN);
    }
  }

  // draw 4 crib cards
  for (int i = 0; i < 4; i++) {
    if (env->crib[i] != 0) {
      DrawTextureEx(env->client->card_back, (Vector2){72 * SCALE + 14 * SCALE * i, 14 * SCALE}, 0,
                    SCALE, GRAY);
      // DrawTextureEx(env->client->cards[env->crib[i]], (Vector2){72*SCALE+14*SCALE*i, 14*SCALE},
      // 0, SCALE, ORANGE);
    }
  }

  // draw up to 9 played cards
  if (env->phase != 0) {
    DrawTextureEx(env->client->cards[env->cut_card], (Vector2){1 * SCALE, 35 * SCALE}, 0, SCALE,
                  WHITE);
    for (int i = 0; i < 8; i++) {
      if (env->played_cards[i] != 0) {
        DrawTextureEx(env->client->cards[env->played_cards[i]],
                      (Vector2){15 * SCALE + 14 * SCALE * i, 35 * SCALE}, 0, SCALE, WHITE);
      }
    }
  }

  // Text
  DrawText(TextFormat("SCORE P1:%d, P2:%d", env->score[0], env->score[1]), 2 * SCALE, 1 * SCALE,
           10 * SCALE, WHITE);
  if (env->phase == 0) {
    DrawText(TextFormat("Press Numbers then"), 12 * SCALE, 30 * SCALE, 10 * SCALE, WHITE);
    DrawText(TextFormat("'ENTER' to Play"), 18 * SCALE, 40 * SCALE, 10 * SCALE, WHITE);
    if (env->player_turn == 0) {
      DrawText(TextFormat("YOUR CRIB, PICK 2"), 2 * SCALE, 54 * SCALE, 10 * SCALE, WHITE);
    } else {
      DrawText(TextFormat("RIVAL CRIB, PICK 2"), 2 * SCALE, 54 * SCALE, 10 * SCALE, WHITE);
    }
  } else {
    DrawText(TextFormat("COUNT: %d", env->count), 2 * SCALE, 55 * SCALE, 10 * SCALE, WHITE);
  }

  // draw 6 player cards
  for (int i = 0; i < 6; i++) {
    if (env->hand[i] != 0) {
      DrawTextureEx(env->client->cards[env->hand[i]],
                    (Vector2){7 * SCALE + 16 * SCALE * i, 66 * SCALE}, 0, SCALE, WHITE);
      DrawText(TextFormat("%d", i + 1), 11 * SCALE + 16 * SCALE * i, 86 * SCALE, 10 * SCALE, WHITE);
    }
  }
  if (env->phase == 0) {
    if (env->client->lasttwo[0] != -1) {
      DrawTextureEx(env->client->cards[env->hand[env->client->lasttwo[0]]],
                    (Vector2){7 * SCALE + 16 * SCALE * env->client->lasttwo[0], 66 * SCALE}, 0,
                    SCALE, GRAY);
      DrawText(TextFormat("%d", env->client->lasttwo[0] + 1),
               11 * SCALE + 16 * SCALE * env->client->lasttwo[0], 86 * SCALE, 10 * SCALE, GRAY);
    }
    if (env->client->lasttwo[1] != -1) {
      DrawTextureEx(env->client->cards[env->hand[env->client->lasttwo[1]]],
                    (Vector2){7 * SCALE + 16 * SCALE * env->client->lasttwo[1], 66 * SCALE}, 0,
                    SCALE, GRAY);
      DrawText(TextFormat("%d", env->client->lasttwo[1] + 1),
               11 * SCALE + 16 * SCALE * env->client->lasttwo[1], 86 * SCALE, 10 * SCALE, GRAY);
    }
  }
  if (env->phase != 0 && env->client->lastone != -1) {
    if (env->hand[env->client->lastone] != 0) {
      DrawTextureEx(env->client->cards[env->hand[env->client->lastone]],
                    (Vector2){7 * SCALE + 16 * SCALE * env->client->lastone, 66 * SCALE}, 0, SCALE,
                    GRAY);
      DrawText(TextFormat("%d", env->client->lastone + 1),
               11 * SCALE + 16 * SCALE * env->client->lastone, 86 * SCALE, 10 * SCALE, GRAY);
    }
  }

  EndDrawing();
}

// Required function. Should clean up anything you allocated
// Do not free env->observations, actions, rewards, terminals
void c_close(Cribbage* env) {
  if (env->client != NULL) {
    Client* client = env->client;
    UnloadTexture(client->card_back);
    for (int i = 0; i < 53; i++) {
      UnloadTexture(client->cards[i]);
    }
    CloseWindow();
    free(client);
  }
}