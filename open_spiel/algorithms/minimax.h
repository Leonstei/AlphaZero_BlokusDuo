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

#ifndef OPEN_SPIEL_ALGORITHMS_MINMAX_H_
#define OPEN_SPIEL_ALGORITHMS_MINMAX_H_

#include <memory>
#include <utility>

#include "spiel_bots.h"
#include "open_spiel/spiel.h"

namespace open_spiel {
namespace algorithms {

    // 3. Deklaration der AlphaBetaSearch mit Iterative Deepening
    // Diese Funktion nutzt ein Zeitlimit anstelle eines festen Tiefenlimits.
    std::pair<double, Action> AlphaBetaSearchID(
        const Game& game,
        const State* state,
        const std::function<double(const State&)>& value_function,
        double max_time_seconds,
        int depth_limit,
        Player maximizing_player,
        bool use_undo);

// Solves deterministic, 2-players, perfect-information 0-sum game.
//
// Arguments:
//   game: The game to analyze, as returned by `LoadGame`.
//   state: The state to start from. If nullptr, starts from initial state.
//   value_function: An optional function mapping a Spiel `State` to a
//     numerical value to the maximizing player, to be used as the value for a
//     node when we reach `depth_limit` and the node is not terminal. Use
//     `nullptr` for no value function.
//   depth_limit: The maximum depth to search over. When this depth is
//     reached, an exception will be raised.
//   maximizing_player_id: The id of the MAX player. The other player is assumed
//     to be MIN. Passing in kInvalidPlayer will set this to the search root's
//     current player.

//   Returns:
//     A pair of the value of the game for the maximizing player when both
//     players play optimally, along with the action that achieves this value.

std::pair<double, Action> AlphaBetaSearch(
    const Game& game, const State* state,
    const std::function<double(const State&)>& value_function, int depth_limit,
    Player maximizing_player, bool use_undo = true);

// Solves stochastic, 2-players, perfect-information 0-sum game.
//
// Arguments:
//   game: The game to analyze, as returned by `LoadGame`.
//   state: The state to start from. If nullptr, starts from initial state.
//   value_function: An optional function mapping a Spiel `State` to a
//     numerical value to the maximizing player, to be used as the value for a
//     node when we reach `depth_limit` and the node is not terminal. Use
//     `nullptr` or {} for no value function.
//   depth_limit: The maximum depth to search over (not counting chance nodes).
//     When this depth is reached, an exception will be raised.
//   maximizing_player_id: The id of the MAX player. The other player is assumed
//     to be MIN. Passing in kInvalidPlayer will set this to the search root's
//     current player (which must not be a chance node).

//   Returns:
//     A pair of the value of the game for the maximizing player when both
//     players play optimally, along with the action that achieves this value.

std::pair<double, Action> ExpectiminimaxSearch(
    const Game& game, const State* state,
    const std::function<double(const State&)>& value_function, int depth_limit,
    Player maximizing_player);

inline double BlokusValueFunction(const State& state)
{
    // Gibt den Wert aus Sicht des aktuellen Spielers zurueck.
    // Z.B. (Eigene_Punkte - Gegner_Punkte) / 100
    // Dies müsste in Ihrer Blokus_Duo Implementierung vorhanden sein!
    // const auto& blokus_state = dynamic_cast<const BlokusDuoState&>(state);
    // return blokus_state.EvaluationFunktion(state.CurrentPlayer());
    return state.PlayerReturn(state.CurrentPlayer());
}

class AlphaBetaBot : public Bot
{
public:
    AlphaBetaBot(const open_spiel::Game& game)
        : game_(game)
    {

    }

    open_spiel::Action Step(const open_spiel::State& state) override
    {
        open_spiel::Player player = state.CurrentPlayer();

        double max_time = 10.0;
        int depth_limit = 10; // maximale Suchtiefe

        auto result = open_spiel::algorithms::AlphaBetaSearchID(
            game_,
            &state,
            BlokusValueFunction, // oder deine Heuristik
            max_time,
            depth_limit,
            player,
           true);

        return result.second; // beste Aktion
    }

private:
    const open_spiel::Game& game_;
};


}  // namespace algorithms
}  // namespace open_spiel

#endif  // OPEN_SPIEL_ALGORITHMS_MINMAX_H_
