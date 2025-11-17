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


#include <algorithm>
#include <array>
#include <memory>
#include <string>
#include <vector>

#include "open_spiel/abseil-cpp/absl/strings/str_cat.h"
#include "open_spiel/abseil-cpp/absl/types/span.h"
#include "open_spiel/game_parameters.h"
#include "open_spiel/observer.h"
#include "open_spiel/spiel.h"
#include "open_spiel/spiel_globals.h"
#include "open_spiel/spiel_utils.h"
#include "open_spiel/utils/tensor_view.h"
#include "absl/strings/str_join.h"
#include "absl/strings/str_cat.h"

#include "open_spiel/games/blokus_duo/blokus_duo.h"
#include "open_spiel/games/blokus_duo/blokus_duo_logic.h"



namespace open_spiel {
namespace blokus_duo {
namespace {


// Facts about the game.
const GameType kGameType{
    /*short_name=*/"blokus_duo",
    /*long_name=*/"Blokus Duo",
    GameType::Dynamics::kSequential,
    GameType::ChanceMode::kDeterministic,
    GameType::Information::kPerfectInformation,
    GameType::Utility::kZeroSum,
    GameType::RewardModel::kTerminal,
    /*max_num_players=*/2,
    /*min_num_players=*/2,
    /*provides_information_state_string=*/true,
    /*provides_information_state_tensor=*/false,
    /*provides_observation_string=*/true,
    /*provides_observation_tensor=*/true,
    /*parameter_specification=*/{}  // no parameters
};

std::shared_ptr<const Game> Factory(const GameParameters& params) {
  return std::shared_ptr<const Game>(new BlokusDuoGame(params));
}

REGISTER_SPIEL_GAME(kGameType, Factory);

RegisterSingleTensorObserver single_tensor(kGameType.short_name);

}  // namespace



void BlokusDuoState::DoApplyAction(open_spiel::Action move) {
  if (move == kPassAction) {
    // Aktion war PASSEN:
    consecutive_passes_++;
  }else
  {
    auto& current_player_board =
      CurrentPlayer() == 0 ? player_0_board_ : player_1_board_;
    auto& current_player_edges =
      CurrentPlayer() == 0 ? player_0_edges : player_1_edges;
    auto& current_polyomino_mask =
      CurrentPlayer() == 0 ? polyomino_mask_player_0 : polyomino_mask_player_1;

    // **Dekodierung:** Direkter Lookup in der globalen Liste
    if (move < 0 || move >= ALL_DISTINCT_ACTIONS.size())
    {
      SpielFatalError(absl::StrCat("Ungültige Aktion: ", move));
    }

    // Abrufen der vollständigen Metadaten und der ShiftedMask
    const Action& blokus_action = ALL_DISTINCT_ACTIONS[move];

    place_piece(blokus_action, combined_board_);
    place_piece(blokus_action, current_player_board);
    update_edges(current_player_edges, blokus_action.shifted);
    update_polyomino_mask(current_polyomino_mask, blokus_action.type);

  }

  current_player_ = 1 - current_player_;
  num_moves_++;
}

std::vector<open_spiel::Action> BlokusDuoState::LegalActions() const {
  if (IsTerminal()) return {};

  const auto& current_player_board =
    CurrentPlayer() == 0 ? player_0_board_ : player_1_board_;
  // const auto& opponent_board =
  //   CurrentPlayer() == 0 ? player_1_board_ : player_0_board_;
  const auto& current_player_edges =
    CurrentPlayer() == 0 ? player_0_edges : player_1_edges;
  const uint32_t& current_polyomino_mask =
    CurrentPlayer() == 0 ? polyomino_mask_player_0 : polyomino_mask_player_1;

  std::vector<open_spiel::Action> moves_indices;

  // Annahme: ALL_DISTINCT_ACTIONS ist global verfügbar.
  for (int i = 0; i < ALL_DISTINCT_ACTIONS.size(); ++i) {
    const Action& action_data = ALL_DISTINCT_ACTIONS[i];

    // 1. Prüfungen (z.B. Stein noch verfügbar)
    if (!HasPolyomino( action_data.type, current_polyomino_mask)) continue;

    // 2. Logische Platzierungsprüfung
    if (is_placement_legal(
      combined_board_,
      bit_border_,
      current_player_edges,
      current_player_board,
      action_data.shifted))
      {
      moves_indices.push_back(i);
    }
  }
  if (moves_indices.empty()) {
    moves_indices.push_back(kPassAction);
  }

  return moves_indices;
}




std::string BlokusDuoState::ActionToString(
  Player player, open_spiel::Action action_id) const {
  return game_->ActionToString(player, action_id);
}

// bool BlokusDuoState::HasLine(Player player) const {
//   return BoardHasLine(board_, player);
// }

// bool BlokusDuoState::IsFull() const { return num_moves_ == max_game_length; }

BlokusDuoState::BlokusDuoState(std::shared_ptr<const Game> game) : State(game) {
  combined_board_.fill(0ULL);
  player_0_board_.fill(0ULL);
  player_1_board_.fill(0ULL);
  bit_border_.fill(0ULL);
  player_0_edges = {0ULL ,67108864, 0ULL, 0ULL};
  player_1_edges = {0ULL , 0ULL, 137438953472ULL, 0ULL};
  polyomino_mask_player_0 = 0x1FFFFF; // maske für ob alle 21 Steine da sind
  polyomino_mask_player_1 = 0x1FFFFF; // maske für ob alle 21 Steine da sind


  // 2. Rand auf dem combined_board_ setzen.
  //    Der Rand repräsentiert die ungültigen Positionen.
  for (int r = 0; r < kBoardSize; ++r)
  {
    for (int c = 0; c < kBoardSize; ++c)
    {
      // Prüfen, ob die Koordinate am Rand (Zeile 0, Zeile 15, Spalte 0, Spalte 15) liegt.
      if (r == 0 || r == kBoardSize - 1 || c == 0 || c == kBoardSize - 1)
      {
        int index = r * kBoardSize + c;

        int part = index / 64;
        int bit = index % 64;

        if (index == 165)
        {
          player_0_edges[part] |= (1ULL << bit);
        }
        if (index == 90)
        {
          player_1_edges[part] |= (1ULL << bit);
        }

        bit_border_[part] |= (1ULL << bit);
      }
    }
  }
}


std::string BlokusDuoState::ToString() const {
  const auto& current_board = (current_player_ == 0) ? player_0_board_ : player_1_board_;
  const auto& opponent_board = (current_player_ == 0) ? player_1_board_ : player_0_board_;

  // Die Board-Darstellung:
  std::string board_str = BoardToString(
      current_board,
      opponent_board,
      'X', // Aktueller Spieler
      'O'  // Gegner
  );

  // Status-Informationen
  return absl::StrCat(
      "--- Blokus Duo State ---\n",
      "Player to move: ", current_player_, "\n",
      "Moves made: ", num_moves_, "\n",
      "Consecutive passes: ", consecutive_passes_, "\n",
      "Player 0 Pieces: ", polyomino_mask_player_0, "\n", // Zeigt die Maske an
      "Player 1 Pieces: ", polyomino_mask_player_1, "\n",
      "Board (X=P", current_player_, ", O=P", 1 - current_player_, "):\n",
      board_str,
      "Is Terminal: ", IsTerminal() ? "Yes" : "No", "\n",
      "Returns: ", absl::StrJoin(Returns(), ", "), "\n"
  );
}

std::unique_ptr<StateStruct> BlokusDuoState::ToStruct() const {
  BlokusDuoStateStruct rv;
  std::vector<std::string> board;
  // board.reserve(board_.size());
  // for (const CellState& cell : board_) {
  //   board.push_back(StateToString(cell));
  // }
  // rv.current_player = PlayerToString(CurrentPlayer());
  // rv.board = board;
  return std::make_unique<BlokusDuoStateStruct>(rv);
}


bool BlokusDuoState::IsTerminal() const {
  return consecutive_passes_ == NumPlayers();
}

std::vector<double> BlokusDuoState::Returns() const {
  if (!IsTerminal()) {
    // Muss nur im terminalen Zustand aufgerufen werden!
    return {0.0, 0.0};
  }
  // Annahme: Sie haben Funktionen, die die Endpunktzahlen liefern
  double score0 = CalculateFinalScore(player_0_board_);
  double score1 = CalculateFinalScore(player_1_board_);

  // 1. Berechnung des Ergebnisses
  if (score0 > score1) {
    return {1.0, -1.0}; // Spieler 0 gewinnt
  } else if (score1 > score0) {
    return {-1.0, 1.0}; // Spieler 1 gewinnt
  } else {
    return {0.0, 0.0};  // Unentschieden
  }
}

double BlokusDuoState::PlayerReturn(Player player) const
{
  if (!IsTerminal())
  {
    return 0.0;
  }
  return Returns()[player];
}

std::string BlokusDuoState::InformationStateString(Player player) const {
  SPIEL_CHECK_GE(player, 0);
  SPIEL_CHECK_LT(player, num_players_);
  return HistoryString();
}

std::string BlokusDuoState::ObservationString(Player player) const {
  SPIEL_CHECK_GE(player, 0);
  SPIEL_CHECK_LT(player, num_players_);
  return ToString();
}

void BlokusDuoState::ObservationTensor(Player player,
                                       absl::Span<float> values) const {
  const auto& current_player_board =
    player == 0 ? player_0_board_ : player_1_board_;
  const auto& opponent_board =
    player == 0 ? player_1_board_ : player_0_board_;
  const auto& current_player_edges =
    CurrentPlayer() == 0 ? player_0_edges : player_1_edges;
  const auto& opponent_edges =
    CurrentPlayer() == 0 ? player_0_edges : player_1_edges;

  const uint32_t current_mask = player == 0 ? polyomino_mask_player_0 : polyomino_mask_player_1;
  const uint32_t opponent_mask = player == 0 ? polyomino_mask_player_1 : polyomino_mask_player_0;

  std::fill(values.begin(), values.end(), 0.0f);

  TensorView<3> view(values, {kTotalChannels, kBoardSize, kBoardSize}, true);
  int current_index = 0;

  for (int r = 0; r < kBoardSize; ++r)
  {
    for (int c = 0; c < kBoardSize; ++c) {

      if (IsBitSet(current_player_board, r, c)) { // Pseudo-Funktion
        view[{0, r, c}] = 1.0f; // Kanal 0: Eigene Steine
      }
      if (IsBitSet(opponent_board, r, c)) { // Pseudo-Funktion
        view[{1, r, c}] = 1.0f; // Kanal 1: Gegner Steine
      }
      if (IsBitSet(current_player_edges, r, c)) { // Pseudo-Funktion
        view[{2, r, c}] = 1.0f; // Kanal 2: Eigene Kanten
      }
      if (IsBitSet(opponent_edges, r, c)) { // Pseudo-Funktion
        view[{3, r, c}] = 1.0f; // Kanal 3: Gegner Kanten
      }
    }
  }
  int current_channel = 4;
  for (int i = 0; i < kNumPolyominoTypes; ++i) { // i = 0 bis 20 (21 Steine)
    float value = (current_mask >> i & 1u) ? 1.0f : 0.0f;
    for (int r = 0; r < kBoardSize; ++r) {
      for (int c = 0; c < kBoardSize; ++c) {
        view[{current_channel, r, c}] = value; // Füllt die gesamte Ebene mit dem gleichen Wert
      }
    }
    current_channel++;
  }

  // 4.2 Gegnerische Steine (Kanäle 25 bis 45)
  for (int i = 0; i < kNumPolyominoTypes; ++i) { // i = 0 bis 20 (21 Steine)
    float value = (opponent_mask >> i & 1u) ? 1.0f : 0.0f;
    for (int r = 0; r < kBoardSize; ++r) {
      for (int c = 0; c < kBoardSize; ++c) {
        view[{current_channel, r, c}] = value; // Füllt die gesamte Ebene mit dem gleichen Wert
      }
    }
    current_channel++;
  }
}

void BlokusDuoState::UndoAction(Player player, open_spiel::Action move) {

}

std::unique_ptr<State> BlokusDuoState::Clone() const {
  return std::unique_ptr<State>(new BlokusDuoState(*this));
}

std::string BlokusDuoGame::ActionToString(Player player, open_spiel::Action action_id) const {
  int index = static_cast<int>(action_id);

  // Prüft, ob es sich um eine der 13.729 regulären Aktionen handelt
  if (index >= 0 && index < ALL_DISTINCT_ACTIONS.size()) {
    const Action& action_data = ALL_DISTINCT_ACTIONS[index];

    // Verwendet absl::StrCat für effizientes String-Concatenation
    return absl::StrCat(
        "P", player, " plays ",
        PolyominoTypeToString(action_data.type),
        " (Var ", action_data.variant_index,
        ") at Pos ", action_data.position,
        " (Index ", index, ")"
    );
  }

  // Fall: Pass-Aktion (angenommen, die Pass-Aktion hat einen festen Index
  // außerhalb der regulären 13.729 Aktionen, z.B. 13729)
  if (action_id == NumDistinctActions() - 1) {
    return absl::StrCat("P", player, " passes.");
  }

  // Fehlerfall
  return absl::StrCat("P", player, " Invalid Action ID: ", action_id);
}

BlokusDuoGame::BlokusDuoGame(const GameParameters& params)
    : Game(kGameType, params) {}

}  // namespace blokus_duo
}  // namespace open_spiel
