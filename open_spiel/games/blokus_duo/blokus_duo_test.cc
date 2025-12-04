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
#include <iostream>
#include <memory>
#include <utility>

#include "open_spiel/json/include/nlohmann/json.hpp"
#include "open_spiel/games/tic_tac_toe/tic_tac_toe.h"
#include "open_spiel/spiel.h"
#include "open_spiel/spiel_utils.h"
#include "open_spiel/tests/basic_tests.h"
#include "open_spiel/algorithms/mcts.h"
#include "open_spiel/algorithms/minimax.h"
#include "open_spiel/abseil-cpp/absl/strings/str_join.h"

namespace open_spiel {
namespace blokus_duo {
namespace {

namespace testing = open_spiel::testing;

void BasicBlokusDuoTests() {
  testing::LoadGameTest("blokus_duo");
  testing::NoChanceOutcomesTest(*LoadGame("blokus_duo"));
  testing::RandomSimTest(*LoadGame("blokus_duo"), 100);


  // auto game = LoadGame("blokus_duo");
  // auto state = game->NewInitialState();


  // std::vector<float> values(game ->ObservationTensorSize());
  // state->ObservationTensor(state->CurrentPlayer(), absl::MakeSpan(values));
  // SPIEL_CHECK_FLOAT_EQ(values[0], 0.0f);
  // const std::vector<int> expected_shape = {kTotalChannels, kBoardSizeWithoutBorder, kBoardSizeWithoutBorder};
  // SPIEL_CHECK_EQ(game->ObservationTensorShape(), expected_shape);
  // const int expected_size = kTotalChannels * kBoardSizeWithoutBorder * kBoardSizeWithoutBorder;
  // SPIEL_CHECK_EQ(game->ObservationTensorSize(), expected_size);
  // SPIEL_CHECK_FLOAT_EQ(values[0], 0.0f);
  // SPIEL_CHECK_FLOAT_EQ(values.back(), 1);
  // --- ENDE NEUER TEST ---

}
  void BenchmarkBlokusPerformance(int num_games) {
  std::cout << "Starte Benchmark fuer " << num_games << " Spiele..." << std::endl;

  auto game = LoadGame("blokus_duo");
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

double BlokusValueFunction(const State& state){
    // Gibt den Wert aus Sicht des aktuellen Spielers zurueck.
    // Z.B. (Eigene_Punkte - Gegner_Punkte) / 100
    // Dies müsste in Ihrer Blokus_Duo Implementierung vorhanden sein!
    // const auto& blokus_state = dynamic_cast<const BlokusDuoState&>(state);
    // return blokus_state.EvaluationFunktion(state.CurrentPlayer());
    return state.PlayerReturn(state.CurrentPlayer());
}
void BlokusMctsVsMinimaxTest() {
  // 1. SPIEL LADEN UND ZUSTAND INITIALISIEREN
  // Stellen Sie sicher, dass "blokus_duo" als Kurzname (Short Name) in Ihrem Spiel registriert ist.
  std::shared_ptr<const Game> game = LoadGame("blokus_duo");
  std::unique_ptr<State> state = game->NewInitialState();

  // 2. AGENTEN INITIALISIEREN
  // A. Evaluator erstellen (nutzt die in mcts.h enthaltene Klasse)
  const int kRolloutsPerEval = 5; // Anzahl der Rollouts zur Bewertung eines Blattes
  const int kSeedEval = 0;


  // Definieren der MCTS-Parameter fuer den Konstruktor
  const double kUctC = 2.0;
  const int kMaxSimulations = 500;
  const int64_t kMaxMemoryMb = 1024;
  const bool kSolve = false; // Fuer Blokus Duo erstmal unnötig
  const int kSeed = 0;
  const bool kVerbose = false;
  // Standard-Policy und Dirichlet-Werte:
  const open_spiel::algorithms::ChildSelectionPolicy kPolicy =
      open_spiel::algorithms::ChildSelectionPolicy::UCT;
  const double kDirichletAlpha = 0.0;
  const double kDirichletEpsilon = 0.0;
  const bool kDontReturnChanceNode = false;
  // Wir erstellen einen shared_ptr auf den RandomRolloutEvaluator
  std::shared_ptr<open_spiel::algorithms::Evaluator> evaluator =
      std::make_shared<open_spiel::algorithms::RandomRolloutEvaluator>(
          kRolloutsPerEval, kSeedEval);
    std::unique_ptr<open_spiel::algorithms::MCTSBot> mcts_bot =
          std::make_unique<open_spiel::algorithms::MCTSBot>(
              *game,
              evaluator,
              kUctC,
              kMaxSimulations,
              kMaxMemoryMb,
              kSolve,
              kSeed,
              kVerbose,
              kPolicy,
              kDirichletAlpha,
              kDirichletEpsilon,
              kDontReturnChanceNode
          );


  while (!state->IsTerminal()) {
    open_spiel::Player current_player = state->CurrentPlayer();

    std::cout << "------------------------------------------" << std::endl;
    std::cout << "Zug von Spieler " << current_player << std::endl;
    std::cout << "Aktueller Zustand:\n" << state->ToString() << std::endl;

    open_spiel::Action action;
      std::vector<open_spiel::Action> actions;

    if (current_player == 0) {
      // MCTS berechnet den besten Zug
      action = mcts_bot->Step(*state);
      std::cout << "MCTS waehlt Aktion: "
                << game->ActionToString(current_player, action) << std::endl;
    } else {
        // std::cout << "minimax started "<< std::endl;
      // Minimax turn: nutzt die statische AlphaBetaSearch Funktion

      int depth_limit = 10; // Maximale Suchtiefe
        double max_time =30.0;

      std::pair<double, open_spiel::Action> result =
          open_spiel::algorithms::AlphaBetaSearchID(
              *game,                  // Das Spiel
              state.get(),            // Der aktuelle Zustand
              BlokusValueFunction,                // Value-Function (NULL, wenn keine Heuristik verwendet wird)
              max_time,
              depth_limit,
              current_player,      // Maximizing Player ist der aktuelle Spieler,
              true
          );

      action = result.second;
      double value = result.first;

      std::cout << "Minimax (Tiefe " << depth_limit << ") waehlt Aktion: "
                << game->ActionToString(current_player, action)
                << " (Erwarteter Wert: " << value << ")" << std::endl;
    }

    // Zug ausführen
    state->ApplyAction(action);
  }

  // 4. ENDE DES SPIELS
  std::cout << "==========================================" << std::endl;
  std::cout << "Spiel beendet. Endzustaende:\n" << state->ToString() << std::endl;
  std::cout << "Ergebnisse (Returns) fuer Spieler 0 und 1: "
            << absl::StrJoin(state->Returns(), ", ") << std::endl;
}


}  // namespace
}  // namespace tic_tac_toe
}  // namespace open_spiel

int main(int argc, char** argv) {
  open_spiel::blokus_duo::BenchmarkBlokusPerformance(10000);
  //open_spiel::blokus_duo::BasicBlokusDuoTests();
  // open_spiel::blokus_duo::TestStateStruct();
  //open_spiel::blokus_duo::BlokusMctsVsMinimaxTest();

}
