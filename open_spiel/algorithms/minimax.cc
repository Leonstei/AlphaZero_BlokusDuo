// Copyright 2021 DeepMind Technologies Limited
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "open_spiel/algorithms/minimax.h"

#include <algorithm>  // std::max
#include <limits>
#include <memory>

#include "open_spiel/spiel.h"
#include "open_spiel/spiel_utils.h"

namespace open_spiel {
namespace algorithms {
namespace {

// An alpha-beta algorithm.
//
// Implements a min-max algorithm with alpha-beta pruning.
// See for example https://en.wikipedia.org/wiki/Alpha-beta_pruning
//
// Arguments:
//   state: The current state node of the game.
//   depth: The maximum depth for the min/max search.
//   alpha: best value that the MAX player can guarantee (if the value is <=
//     alpha, the MAX player will avoid it).
//   beta: the best value that the MIN currently can guarantee (if the value is
//     >= than beta, the MIN player will avoid it).
//   value_function: An optional function mapping a Spiel `State` to a
//     numerical value, to be used as the value for a node when we reach
//     `depth_limit` and the node is not terminal.
//   maximizing_player_id: The id of the MAX player. The other player is assumed
//     to be MIN.
//   use_undo: use the State::Undo for faster run-time.
//
// Returns:
//   The optimal value of the sub-game starting in state (given alpha/beta).
double _alpha_beta(State* state, int depth, double alpha, double beta,
                   const std::function<double(const State&)>& value_function,
                   Player maximizing_player, Action* best_action,
                   bool use_undo) {
  if (state->IsTerminal()) {
    return state->PlayerReturn(maximizing_player);
  }

  if (depth == 0 && !value_function) {
    SpielFatalError(
        "We assume we can walk the full depth of the tree. "
        "Try increasing depth or provide a value_function.");
  }

  if (depth == 0) {
    return state->PlayerReturn(maximizing_player);
  }

  Player player = state->CurrentPlayer();
  if (player == maximizing_player) {
    double value = -std::numeric_limits<double>::infinity();

    for (Action action : state->LegalActions()) {
      double child_value = 0;
      if (use_undo) {
        state->ApplyAction(action);
        child_value =
            _alpha_beta(state, /*depth=*/depth - 1, /*alpha=*/alpha,
                        /*beta=*/beta, value_function, maximizing_player,
                        /*best_action=*/nullptr, use_undo);
        state->UndoAction(player, action);
      } else {
        std::unique_ptr<State> child_state = state->Child(action);
        child_value =
            _alpha_beta(child_state.get(), /*depth=*/depth - 1, /*alpha=*/alpha,
                        /*beta=*/beta, value_function, maximizing_player,
                        /*best_action=*/nullptr, use_undo);
      }

      if (child_value > value) {
        value = child_value;
        if (best_action != nullptr) {
          *best_action = action;
        }
      }

      alpha = std::max(alpha, value);
      if (alpha >= beta) {
        break;  // beta cut-off
      }
    }

    return value;
  } else {
    double value = std::numeric_limits<double>::infinity();

    for (Action action : state->LegalActions()) {
      double child_value = 0;
      if (use_undo) {
        state->ApplyAction(action);
        child_value =
            _alpha_beta(state, /*depth=*/depth - 1, /*alpha=*/alpha,
                        /*beta=*/beta, value_function, maximizing_player,
                        /*best_action=*/nullptr, use_undo);
        state->UndoAction(player, action);
      } else {
        std::unique_ptr<State> child_state = state->Child(action);
        child_value =
            _alpha_beta(child_state.get(), /*depth=*/depth - 1, /*alpha=*/alpha,
                        /*beta=*/beta, value_function, maximizing_player,
                        /*best_action=*/nullptr, use_undo);
      }

      if (child_value < value) {
        value = child_value;
        if (best_action != nullptr) {
          *best_action = action;
        }
      }

      beta = std::min(beta, value);
      if (alpha >= beta) {
        break;  // alpha cut-off
      }
    }

    return value;
  }
}



// Expectiminimax algorithm.
//
// Runs expectiminimax until the specified depth.
// See https://en.wikipedia.org/wiki/Expectiminimax for details.
//
// Arguments:
//   state: The state to start the search from.
//   depth: The depth of the search (not counting chance nodes).
//   value_function: A value function, taking in a state and returning a value,
//     in terms of the maximizing_player_id.
//   maximizing_player_id: The player running the search (current player at root
//     of the search tree).
//
// Returns:
//   The optimal value of the sub-game starting in state.
double _expectiminimax(const State* state, int depth,
                       const std::function<double(const State&)>& value_function,
                       Player maximizing_player, Action* best_action) {
  if (state->IsTerminal()) {
    return state->PlayerReturn(maximizing_player);
  }

  if (depth == 0 && !value_function) {
    SpielFatalError(
        "We assume we can walk the full depth of the tree. "
        "Try increasing depth or provide a value_function.");
  }

  if (depth == 0) {
    return value_function(*state);
  }

  Player player = state->CurrentPlayer();
  if (state->IsChanceNode()) {
    double value = 0;
    for (const auto& actionprob : state->ChanceOutcomes()) {
      std::unique_ptr<State> child_state = state->Child(actionprob.first);
      double child_value =
          _expectiminimax(child_state.get(), depth, value_function,
                          maximizing_player, /*best_action=*/nullptr);
      value += actionprob.second * child_value;
    }
    return value;
  } else if (player == maximizing_player) {
    double value = -std::numeric_limits<double>::infinity();

    for (Action action : state->LegalActions()) {
      std::unique_ptr<State> child_state = state->Child(action);
      double child_value = _expectiminimax(child_state.get(),
                                           /*depth=*/depth - 1, value_function,
                                           maximizing_player,
                                           /*best_action=*/nullptr);

      if (child_value > value) {
        value = child_value;
        if (best_action != nullptr) {
          *best_action = action;
        }
      }
    }
    return value;
  } else {
    double value = std::numeric_limits<double>::infinity();

    for (Action action : state->LegalActions()) {
      std::unique_ptr<State> child_state = state->Child(action);
      double child_value = _expectiminimax(child_state.get(),
                                           /*depth=*/depth - 1, value_function,
                                           maximizing_player,
                                           /*best_action=*/nullptr);

      if (child_value < value) {
        value = child_value;
        if (best_action != nullptr) {
          *best_action = action;
        }
      }
    }
    return value;
  }
}
}  // namespace
  std::pair<double, Action> AlphaBetaSearchID(
    const open_spiel::Game& game,
    const open_spiel::State* state,
    const std::function<double(const open_spiel::State&)>& value_function,
    double max_time_seconds,
    int depth_limit,
    open_spiel::Player maximizing_player,
    bool use_undo)
{
    // Die beste gefundene Aktion aus der letzten vollen Tiefe
    open_spiel::Action best_action_overall = kInvalidAction;
    double best_value_overall = -std::numeric_limits<double>::infinity();

    if (state->LegalActions().empty())
    {
      std::cout << "actions are empty" << std::endl;
    }
    // Pruefen, ob es ueberhaupt Zuege gibt (z.B. bei Terminalzustand)
    if (state->IsTerminal() || state->LegalActions().empty())
    {
      return {state->PlayerReturn(maximizing_player), kInvalidAction};
    }

    auto start_time = std::chrono::high_resolution_clock::now();
    int depth = 1;

    while (true) {
      auto current_start = std::chrono::high_resolution_clock::now();

      if (depth > depth_limit) {
        std::cout << "DEBUG: Maximale Tiefe (" << depth_limit << ") erreicht." << std::endl;
        break;
      }

        // 1. Alpha-Beta-Suche fuer die aktuelle Tiefe durchführen
      std::pair<double, open_spiel::Action> result_current_depth =
          AlphaBetaSearch(
              game,
              state,
              value_function,
              depth, // <--- Die Tiefe steigt hier (z.B. 1, 2, 3...)
              maximizing_player,
              use_undo
          );

      auto current_end = std::chrono::high_resolution_clock::now();
      std::chrono::duration<double> duration_total = current_end - start_time;
      std::chrono::duration<double> duration_current_depth = current_end - start_time;

      best_action_overall = result_current_depth.second;
      best_value_overall = result_current_depth.first;

      std::cout << "DEBUG: Tiefe " << depth << " abgeschlossen in "
        << duration_current_depth.count() << "s. Gesamtzeit: "
        << duration_total.count() << "s." << std::endl;


      // 3. Abbruchbedingung (Zeitlimit erreicht)
      std::chrono::duration<double> duration_next_search =
        duration_current_depth * 60.0; // Schätzung fuer die nächste Tiefe (Verdopplung)

      if (duration_total.count() + duration_next_search.count() >= max_time_seconds)
      {
        std::cout << "DEBUG: Zeitlimit erreicht. Abbruch nach Tiefe " << depth << std::endl;
        break;
      }

      depth++;
    }

    // 4. Rueckgabe des besten Zuges aus der letzten VOLLSTAENDIGEN Tiefe
    return {best_value_overall, best_action_overall};
}

std::pair<double, Action> AlphaBetaSearch(
    const Game& game, const State* state,
    const std::function<double(const State&)>& value_function, int depth_limit,
    Player maximizing_player, bool use_undo) {
  SPIEL_CHECK_LE(game.NumPlayers(), 2);

  // Check to ensure the correct setup intended for this algorithm.
  // Note: do no check perfect vs. imperfect information to support use of
  // minimax as a subroutine of PIMC.
  const GameType& game_info = game.GetType();
  // SPIEL_CHECK_EQ(game_info.chance_mode, GameType::ChanceMode::kDeterministic);
  // SPIEL_CHECK_EQ(game_info.dynamics, GameType::Dynamics::kSequential);
  // SPIEL_CHECK_EQ(game_info.utility, GameType::Utility::kZeroSum);
  // SPIEL_CHECK_EQ(game_info.reward_model, GameType::RewardModel::kTerminal);

  std::unique_ptr<State> search_root;
  if (state == nullptr) {
    search_root = game.NewInitialState();
  } else {
    search_root = state->Clone();
  }

  if (maximizing_player == kInvalidPlayer) {
    maximizing_player = search_root->CurrentPlayer();
  }

  double infinity = std::numeric_limits<double>::infinity();
  Action best_action = kInvalidAction;
  double value = _alpha_beta(
      search_root.get(), /*depth=*/depth_limit, /*alpha=*/-infinity,
      /*beta=*/infinity, value_function, maximizing_player, &best_action,
      use_undo);

  return {value, best_action};
}

std::pair<double, Action> ExpectiminimaxSearch(
    const Game& game, const State* state,
    const std::function<double(const State&)>& value_function, int depth_limit,
    Player maximizing_player) {
  SPIEL_CHECK_LE(game.NumPlayers(), 2);

  const GameType& game_info = game.GetType();
  SPIEL_CHECK_EQ(game_info.chance_mode,
                 GameType::ChanceMode::kExplicitStochastic);
  SPIEL_CHECK_EQ(game_info.information,
                 GameType::Information::kPerfectInformation);
  SPIEL_CHECK_EQ(game_info.dynamics, GameType::Dynamics::kSequential);
  SPIEL_CHECK_EQ(game_info.utility, GameType::Utility::kZeroSum);
  SPIEL_CHECK_EQ(game_info.reward_model, GameType::RewardModel::kTerminal);

  std::unique_ptr<State> search_root;
  if (state == nullptr) {
    search_root = game.NewInitialState();
  } else {
    search_root = state->Clone();
  }

  if (maximizing_player == kInvalidPlayer) {
    SPIEL_CHECK_FALSE(search_root->IsChanceNode());
    maximizing_player = search_root->CurrentPlayer();
  }

  Action best_action = kInvalidAction;
  double value =
      _expectiminimax(search_root.get(), /*depth=*/depth_limit, value_function,
                      maximizing_player, &best_action);

  return {value, best_action};
}

}  // namespace algorithms
}  // namespace open_spiel
