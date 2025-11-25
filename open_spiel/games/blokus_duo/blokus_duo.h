// Copyright 2019 DeepMind Technologies Limited
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

#ifndef OPEN_SPIEL_GAMES_BLOKUS_DUO_BLOKUS_DUO_H_
#define OPEN_SPIEL_GAMES_BLOKUS_DUO_BLOKUS_DUO_H_

#include <array>
#include <memory>
#include <string>
#include <vector>


#include "open_spiel/abseil-cpp/absl/types/span.h"
#include "open_spiel/json/include/nlohmann/json.hpp"
#include "open_spiel/spiel_utils.h"
#include "open_spiel/spiel.h"
#include "open_spiel/games/blokus_duo/blokus_duo_logic.h"

// Simple game of Noughts and Crosses:
// https://en.wikipedia.org/wiki/Tic-tac-toe
//
// Parameters: none

namespace open_spiel {
namespace blokus_duo {



// https://math.stackexchange.com/questions/485752/tictactoe-state-space-choose-calculation/485852
// inline constexpr int kNumberStates = 5478;



struct BlokusDuoStateStruct : StateStruct {
  int current_player;
  std::vector<uint64_t> board;

  BlokusDuoStateStruct() = default;
  explicit BlokusDuoStateStruct(const std::string& json_str) {
    nlohmann::json::parse(json_str).get_to(*this);
  }

  nlohmann::json to_json_base() const override {
    return *this;
  }
  NLOHMANN_DEFINE_TYPE_INTRUSIVE(BlokusDuoStateStruct, current_player, board);
};

// State of an in-play game.
class BlokusDuoState : public State {
 public:
  BlokusDuoState(std::shared_ptr<const Game> game);

  BlokusDuoState(const BlokusDuoState&) = default;
  BlokusDuoState& operator=(const BlokusDuoState&) = default;

  Player CurrentPlayer() const override {
    return IsTerminal() ? kTerminalPlayerId : current_player_;
  }
  std::string ActionToString(Player player, open_spiel::Action action_id) const override;
  std::string ToString() const override;
  bool IsTerminal() const override;
  std::vector<double> Returns() const override;
  double PlayerReturn(Player player) const override;
  double EvaluationFunktion(Player player) const;
  std::string InformationStateString(Player player) const override;
  std::string ObservationString(Player player) const override;
  void ObservationTensor(Player player,
                         absl::Span<float> values) const override;
  std::unique_ptr<State> Clone() const override;
  void UndoAction(Player player, open_spiel::Action move) override;
  std::vector<open_spiel::Action> LegalActions() const override;
  Player outcome() const { return outcome_; }
  //void ChangePlayer() {current_player_ = current_player_ == 0 ? 1 : 0;}

  void SetCurrentPlayer(Player player) { current_player_ = player; }

  std::unique_ptr<StateStruct> ToStruct() const override;

 protected:
  std::array<uint64_t, kNumBitboardParts> combined_board_;
  std::array<uint64_t, kNumBitboardParts> player_0_board_;
  std::array<uint64_t, kNumBitboardParts> player_1_board_;
  std::array<uint64_t, kNumBitboardParts> bit_border_;
  std::array<uint64_t, kNumBitboardParts> player_0_edges;
  std::array<uint64_t, kNumBitboardParts> player_1_edges;
  uint32_t polyomino_mask_player_0;
  uint32_t polyomino_mask_player_1;


  void DoApplyAction(open_spiel::Action move) override;

 private:
  // bool IsFull() const;                // Is the board full?
  Player current_player_ = 0;         // Player zero goes first
  Player outcome_ = kInvalidPlayer;
  int num_moves_ = 0;
  bool player0_pass = false;
  bool player1_pass = false;
};

// Game object.
class BlokusDuoGame : public Game {
 public:
  explicit BlokusDuoGame(const GameParameters& params);
  int NumDistinctActions() const override { return kNumDistinctActions; }
  std::unique_ptr<State> NewInitialState() const override {
    return std::unique_ptr<State>(new BlokusDuoState(shared_from_this()));
  }
  int NumPlayers() const override { return kNumPlayers; }
  double MinUtility() const override { return -1; }
  absl::optional<double> UtilitySum() const override { return 0; }
  double MaxUtility() const override { return 1; }
  std::vector<int> ObservationTensorShape() const override {// 46
    return {kTotalChannels, kBoardSizeWithoutBorder, kBoardSizeWithoutBorder};
    // Ergebnis: {46, 14, 14}
  }
  int MaxGameLength() const override { return max_game_length; }
  std::string ActionToString(Player player, open_spiel::Action action_id) const override;
};



// inline std::ostream& operator<<(std::ostream& stream, const CellState& state) {
//   return stream << StateToString(state);
// }

}  // namespace blokus_duo
}  // namespace open_spiel

#endif  // OPEN_SPIEL_GAMES_BLOKUS_DUO_BLOKUS_DUO_H_
