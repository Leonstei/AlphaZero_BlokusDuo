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

#ifndef OPEN_SPIEL_ALGORITHMS_ALPHA_ZERO_TORCH_VPEVALUATOR_H_
#define OPEN_SPIEL_ALGORITHMS_ALPHA_ZERO_TORCH_VPEVALUATOR_H_

#include <fstream>
#include <future>  // NOLINT
#include <vector>

#include "open_spiel/abseil-cpp/absl/hash/hash.h"
#include "open_spiel/algorithms/alpha_zero_torch/device_manager.h"
#include "open_spiel/algorithms/alpha_zero_torch/vpnet.h"
#include "open_spiel/algorithms/mcts.h"
#include "open_spiel/spiel.h"
#include "open_spiel/utils/lru_cache.h"
#include "open_spiel/utils/stats.h"
#include "open_spiel/utils/thread.h"
#include "open_spiel/utils/threaded_queue.h"
#include "utils/logger.h"

namespace open_spiel {
namespace algorithms {
namespace torch_az {

    static std::mutex g_log_mutex;
    static std::ofstream g_log_file("gpu_metrics.csv", std::ios::app);

    static void LogCsv(const std::string& tag, double value) {
        using clock = std::chrono::high_resolution_clock;
        auto t = clock::now().time_since_epoch();
        long long ms = std::chrono::duration_cast<std::chrono::milliseconds>(t).count();

        std::lock_guard<std::mutex> lk(g_log_mutex);

        if (!g_log_file.is_open()) {
            // Dieser Fall sollte eigentlich nicht passieren, aber zur Sicherheit:
            g_log_file.open("gpu_metrics.csv", std::ios::app);
        }

        g_log_file << ms << "," << tag << "," << std::fixed << std::setprecision(6) << value << "\n";
        g_log_file.flush();  // stellt sicher, dass crashs die Logs nicht verlieren
    }

class VPNetEvaluator : public Evaluator {
 public:
  explicit VPNetEvaluator(DeviceManager* device_manager, int batch_size,
                          int threads, int cache_size, int cache_shards = 1, Logger* logger = nullptr);
  ~VPNetEvaluator() override;

  // Return a value of this state for each player.
  std::vector<double> Evaluate(const State& state) override;

  // Return a policy: the probability of the current player playing each action.
  ActionsAndProbs Prior(const State& state) override;

  void ClearCache();
  LRUCacheInfo CacheInfo();

  void ResetBatchSizeStats();
  open_spiel::BasicStats BatchSizeStats();
  open_spiel::HistogramNumbered BatchSizeHistogram();

 private:
    Logger* logger_;
  VPNetModel::InferenceOutputs Inference(const State& state);

  void Runner();

  DeviceManager& device_manager_;
  std::vector<std::unique_ptr<LRUCache<uint64_t, VPNetModel::InferenceOutputs>>>
      cache_;
  const int batch_size_;

  struct QueueItem {
    VPNetModel::InferenceInputs inputs;
    std::promise<VPNetModel::InferenceOutputs>* prom;
  };

  ThreadedQueue<QueueItem> queue_;
  StopToken stop_;
  std::vector<Thread> inference_threads_;
  absl::Mutex inference_queue_m_;  // Only one thread at a time should pop.

  absl::Mutex stats_m_;
  open_spiel::BasicStats batch_size_stats_;
  open_spiel::HistogramNumbered batch_size_hist_;
};

}  // namespace torch_az
}  // namespace algorithms
}  // namespace open_spiel

#endif  // OPEN_SPIEL_ALGORITHMS_ALPHA_ZERO_TORCH_VPEVALUATOR_H_
