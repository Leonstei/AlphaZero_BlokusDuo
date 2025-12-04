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

#include "open_spiel/games/connect_four/connect_four.h"
#include <string>

#include "open_spiel/json/include/nlohmann/json.hpp"
#include "open_spiel/spiel.h"
#include "open_spiel/spiel_utils.h"
#include "open_spiel/tests/basic_tests.h"

namespace open_spiel {
namespace connect_four {
namespace {

namespace testing = open_spiel::testing;

void BasicConnectFourTests() {
  testing::LoadGameTest("connect_four");
  testing::NoChanceOutcomesTest(*LoadGame("connect_four"));
  testing::RandomSimTest(*LoadGame("connect_four"), 100);
}

void FastLoss() {
  std::shared_ptr<const Game> game = LoadGame("connect_four");
  auto state = game->NewInitialState();
  state->ApplyAction(3);
  state->ApplyAction(3);
  state->ApplyAction(4);
  state->ApplyAction(4);
  state->ApplyAction(2);
  state->ApplyAction(2);
  SPIEL_CHECK_FALSE(state->IsTerminal());
  state->ApplyAction(1);
  SPIEL_CHECK_TRUE(state->IsTerminal());
  SPIEL_CHECK_EQ(state->Returns(), (std::vector<double>{1.0, -1.0}));
  SPIEL_CHECK_EQ(state->ToString(),
                 ".......\n"
                 ".......\n"
                 ".......\n"
                 ".......\n"
                 "..ooo..\n"
                 ".xxxx..\n");
}

void BasicSerializationTest() {
  std::shared_ptr<const Game> game = LoadGame("connect_four");
  std::unique_ptr<State> state = game->NewInitialState();
  std::unique_ptr<State> state2 = game->DeserializeState(state->Serialize());
  SPIEL_CHECK_EQ(state->ToString(), state2->ToString());
}

void CheckFullBoardDraw() {
  std::shared_ptr<const Game> game = LoadGame("connect_four");
  ConnectFourState state(game,
      "ooxxxoo\n"
      "xxoooxx\n"
      "ooxxxoo\n"
      "xxoooxx\n"
      "ooxxxoo\n"
      "xxoooxx\n");
  SPIEL_CHECK_EQ(state.ToString(),
                 "ooxxxoo\n"
                 "xxoooxx\n"
                 "ooxxxoo\n"
                 "xxoooxx\n"
                 "ooxxxoo\n"
                 "xxoooxx\n");
  SPIEL_CHECK_TRUE(state.IsTerminal());
  SPIEL_CHECK_EQ(state.Returns(), (std::vector<double>{0, 0}));
}
  void BenchmarkBlokusPerformance(int num_games) {
  std::cout << "Starte Benchmark fuer " << num_games << " Spiele..." << std::endl;

  auto game = LoadGame("connect_four");
  std::mt19937 rng(12345); // Fester Seed für Reproduzierbarkeit

  // Timer starten
  auto start_time = std::chrono::high_resolution_clock::now();

  long long total_moves = 0;

  for (int i = 0; i < num_games; ++i) {
    std::unique_ptr<State> state = game->NewInitialState();

    while (!state->IsTerminal()) {
      // 1. Legal Actions holen (Das ist oft der Flaschenhals bei Blokus!)
      std::vector<open_spiel::Action> actions = state->LegalActions();

      if (actions.empty()) {
        // Sollte bei Blokus eigentlich nicht passieren bevor Terminal,
        // außer bei Pass-Moves, aber sicherheitshalber:
        break;
      }

      // 2. Zufälligen Zug wählen
      std::uniform_int_distribution<int> dist(0, actions.size() - 1);
      open_spiel::Action action = actions[dist(rng)];

      if (total_moves == 110)
      {
        std::cout << state->HistoryString() << std::endl;
      }
      // 3. Zug anwenden
      state->ApplyAction(action);
      total_moves++;
    }
  }

  // Timer stoppen
  auto end_time = std::chrono::high_resolution_clock::now();
  std::chrono::duration<double> diff = end_time - start_time;

  double total_seconds = diff.count();
  double games_per_second = num_games / total_seconds;
  double moves_per_second = total_moves / total_seconds;

  std::cout << "------------------------------------------------" << std::endl;
  std::cout << "Zeit gesamt:       " << total_seconds << " s" << std::endl;
  std::cout << "Anzahl Spiele:     " << num_games << std::endl;
  std::cout << "Anzahl Züge:       " << total_moves << std::endl;
  std::cout << "Durschn. Züge/Spiel: " << (double)total_moves / num_games << std::endl;
  std::cout << "------------------------------------------------" << std::endl;
  std::cout << "Spiele pro Sekunde: " << games_per_second << " (Wichtig!)" << std::endl;
  std::cout << "Züge pro Sekunde:   " << moves_per_second << std::endl;
  std::cout << "------------------------------------------------" << std::endl;
}
void TestStateStruct() {
  auto game = LoadGame("connect_four");
  auto state = game->NewInitialState();
  ConnectFourState* cf_state = static_cast<ConnectFourState*>(state.get());
  SPIEL_CHECK_EQ(cf_state->ToStruct()->ToJson(), cf_state->ToJson());
  cf_state->ApplyAction(3);
  cf_state->ApplyAction(4);
  std::string state_json =
      R"({"board":[)"
      R"([".",".",".","x","o",".","."],)"
      R"([".",".",".",".",".",".","."],)"
      R"([".",".",".",".",".",".","."],)"
      R"([".",".",".",".",".",".","."],)"
      R"([".",".",".",".",".",".","."],)"
      R"([".",".",".",".",".",".","."]],)"
      R"("current_player":"x","is_terminal":false,"winner":""})";
  SPIEL_CHECK_EQ(cf_state->ToJson(), state_json);
  cf_state->ApplyAction(3);
  cf_state->ApplyAction(4);
  cf_state->ApplyAction(3);
  cf_state->ApplyAction(4);
  cf_state->ApplyAction(3);
  state_json =
      R"({"board":[)"
      R"([".",".",".","x","o",".","."],)"
      R"([".",".",".","x","o",".","."],)"
      R"([".",".",".","x","o",".","."],)"
      R"([".",".",".","x",".",".","."],)"
      R"([".",".",".",".",".",".","."],)"
      R"([".",".",".",".",".",".","."]],)"
      R"("current_player":"Terminal","is_terminal":true,"winner":"x"})";
  SPIEL_CHECK_EQ(cf_state->ToJson(), state_json);
  SPIEL_CHECK_EQ(nlohmann::json::parse(state_json).dump(),
                 ConnectFourStateStruct(state_json).ToJson());
}

}  // namespace
}  // namespace connect_four
}  // namespace open_spiel

int main(int argc, char **argv) {
  open_spiel::connect_four::BenchmarkBlokusPerformance(100000);
  // open_spiel::connect_four::BasicConnectFourTests();
  // open_spiel::connect_four::FastLoss();
  // open_spiel::connect_four::BasicSerializationTest();
  // open_spiel::connect_four::CheckFullBoardDraw();
  // open_spiel::connect_four::TestStateStruct();
}
