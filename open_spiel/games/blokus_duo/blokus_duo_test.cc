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

#include "blokus_duo.h"

#include <string>

#include "open_spiel/json/include/nlohmann/json.hpp"
#include "open_spiel/games/tic_tac_toe/tic_tac_toe.h"
#include "open_spiel/spiel.h"
#include "open_spiel/spiel_utils.h"
#include "open_spiel/tests/basic_tests.h"

namespace open_spiel {
namespace blokus_duo {
namespace {

namespace testing = open_spiel::testing;

void BasicBlokusDuoTests() {
  testing::LoadGameTest("blokus_duo");
  testing::NoChanceOutcomesTest(*LoadGame("blokus_duo"));
  testing::RandomSimTest(*LoadGame("blokus_duo"), 100);
  auto game = LoadGame("blokus_duo");
  auto state = game->NewInitialState();


  std::vector<float> values(game ->ObservationTensorSize());
  state->ObservationTensor(state->CurrentPlayer(), absl::MakeSpan(values));

  // Der erste Wert (values[0]) korrespondiert mit Kanal 0 (eigene Steine).
  // Das Feld ist leer, also sollte das 0.0f sein.
  SPIEL_CHECK_FLOAT_EQ(values[0], 0.0f);

  // Die erwartete Form des Tensors: {Kanäle, Höhe, Breite}
  const std::vector<int> expected_shape = {kTotalChannels, kBoardSize, kBoardSize};
  std::vector<int> lol = expected_shape;

  // 1. Testet, ob die deklarierte Form korrekt ist
  SPIEL_CHECK_EQ(game->ObservationTensorShape(), expected_shape);

  // 2. Testet, ob die tatsächliche Größe des Tensors korrekt ist (46 * 16 * 16 = 11776)
  const int expected_size = kTotalChannels * kBoardSize * kBoardSize;
  SPIEL_CHECK_EQ(game->ObservationTensorSize(), expected_size);

  // 3. Testet, ob die ObservationTensor-Funktion korrekt die richtige Größe füllt
  // std::vector<float> values(game->ObservationTensorSize());
  // state->ObservationTensor(state->CurrentPlayer(), absl::MakeSpan(values));

  // Da der initiale Zustand leer ist, sollten alle Werte 0.0f sein.
  // Wir prüfen nur den ersten und letzten Wert als Stichprobe:
  SPIEL_CHECK_FLOAT_EQ(values[0], 0.0f);
  SPIEL_CHECK_FLOAT_EQ(values.back(), 0.0f);

  // --- ENDE NEUER TEST ---

}

void TestStateStruct() {
  auto game = LoadGame("blokus_duo");
  auto state = game->NewInitialState();
  BlokusDuoState* ttt_state = static_cast<BlokusDuoState*>(state.get());
  // auto state_struct = ttt_state->ToStruct();
  // // Test state/state_struct -> json string.
  // SPIEL_CHECK_EQ(state_struct->ToJson(), ttt_state->ToJson());
  // std::string state_json =
  //     "{\"board\":[\".\",\".\",\".\",\".\",\".\",\".\",\".\",\".\",\".\"],"
  //     "\"current_player\":\"x\"}";
  // SPIEL_CHECK_EQ(state_struct->ToJson(), state_json);
  // // Test json string -> state_struct.
  // SPIEL_CHECK_EQ(nlohmann::json::parse(state_json).dump(),
  //                BlokusDuoStateStruct(state_json).ToJson());
}

}  // namespace
}  // namespace tic_tac_toe
}  // namespace open_spiel

int main(int argc, char** argv) {
  open_spiel::blokus_duo::BasicBlokusDuoTests();
  open_spiel::blokus_duo::TestStateStruct();
}
