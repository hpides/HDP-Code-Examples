/**
 * Compile prefetching experiment (to write CSV files for plotting, append `-DSERVER`):
 *   clang++ Prefetching.cpp -O3 -o prefetching -std=c++20
 *   
 * To check which prefetching instructions are emitted, append `-S` to command above to output the generated code.
 */

#include <unistd.h>  // gethostname()

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <deque>
#include <iostream>
#include <locale>
#include <map>
#include <fstream>
#include <random>
#include <utility>
#include <vector>


constexpr auto RANDOM_ACCESS_COUNT = size_t{10'000'000};

// ~4 GB. Should be large enough that we do not need to care about caching effects between runs.
auto VECTOR_SIZE = size_t{1'000'000'000};  
auto MEASUREMENT_COUNT = size_t{5};

template <bool PREFETCH, int LOCALITY>
std::pair<size_t, std::chrono::duration<double>> run_experiment(const std::vector<uint32_t>& vec,
                                                                const std::vector<uint64_t>& positions,
                                                                uint16_t prefetch_offset) {
  static_assert((LOCALITY == -1 and !PREFETCH) || (LOCALITY >= 0 && LOCALITY < 4));

  auto sum = size_t{0};
  const auto position_list_size = positions.size();

  const auto start = std::chrono::steady_clock::now();  
  for (auto pos_list_offset = size_t{0}; pos_list_offset < position_list_size; ++pos_list_offset) {
    if constexpr (PREFETCH) {
      __builtin_prefetch(&vec[positions[std::min(position_list_size - 1, pos_list_offset + prefetch_offset)]],
                         0, LOCALITY);
    }

    sum += vec[positions[pos_list_offset]] % 17;
  }
  const auto end = std::chrono::steady_clock::now();
  const auto duration = std::chrono::duration<double>(end - start);

  return {sum, duration};
}

int main() {
  const auto offsets = std::vector<uint16_t>{1, 2, 4, 8, 16, 32, 64, 128, 256};

  #ifdef SERVER
      MEASUREMENT_COUNT *= 10;
      VECTOR_SIZE *= 4;
  #endif

  #ifdef LARGE
      VECTOR_SIZE *= 14;
  #endif

  std::cout << std::format("Running {:L} random accesses in {} runs on {:L} MB data vector.\n",
                           RANDOM_ACCESS_COUNT, MEASUREMENT_COUNT, (sizeof(uint32_t)*VECTOR_SIZE)/1000/1000);

  auto random_engine = std::default_random_engine();
  auto value_distribution = std::uniform_int_distribution<uint32_t>(0);
  auto position_distribution = std::uniform_int_distribution<uint64_t>(0, VECTOR_SIZE - 1);

  std::cout << "Creating data vector ... " << std::flush;
  auto vec = std::vector<uint32_t>(VECTOR_SIZE);
  for (auto index = size_t{0}; index < VECTOR_SIZE; ++index) {
    vec[index] = value_distribution(random_engine);
  }
  std::cout << "done.\n";

  std::cout << "Creating position list ... " << std::flush;
  auto access_positions = std::vector<uint64_t>(RANDOM_ACCESS_COUNT);
  for (auto index = size_t{0}; index < RANDOM_ACCESS_COUNT; ++index) {
    access_positions[index] = position_distribution(random_engine);
  }
  std::cout << "done.\n";

  auto random_device = std::random_device{};
  auto generator = std::mt19937{random_device()};
  auto results = std::map<std::tuple<std::string, int, uint16_t>, std::deque<double>>{};
  std::cout << "Running measurement run" << std::flush;
  for (auto measurement_id = size_t{0}; measurement_id < MEASUREMENT_COUNT + 1; ++measurement_id) {  
    std::cout << " #" << measurement_id << std::flush;
    std::shuffle(access_positions.begin(), access_positions.end(), generator);

    
    for (auto locality = int{-1}; locality < 4; ++locality) {
      // Get result from no-prefetching run.
      const auto& [no_prefetch_result, no_prefetch_runtime] = run_experiment<false, -1>(vec, access_positions, 0);

      for (const auto offset : offsets) {
        auto result = std::pair<size_t, std::chrono::duration<double>>{};
        switch (locality) {
          case -1: {
            result = run_experiment<false, -1>(vec, access_positions, 0);
            break;
          }
          case 0: {
            result = run_experiment<true, 0>(vec, access_positions, offset);
            break;
          }
          case 1: {
            result = run_experiment<true, 1>(vec, access_positions, offset);
            break;
          }
          case 2: {
            result = run_experiment<true, 2>(vec, access_positions, offset);
            break;
          }
          case 3: {
            result = run_experiment<true, 3>(vec, access_positions, offset);
            break;
          }
        }
        if (no_prefetch_result != result.first) {
          std::cerr << "Unexpected result.\n";
          std::exit(1);
        }

        if (measurement_id > 0) { // Skip warmup round.
          results[{locality < 0 ? "No Prefetching" : "Prefetching", locality, offset}].push_back(result.second.count());
        }
      }
    }
  }
  std::cout << std::endl;

  for (const auto& [key, runtimes] : results) {
    std::cout << std::format("{: <16} - Locality: {: } - Offset: {:3} - Avg. runtime: {:2.5f} s\n", std::get<0>(key),
                 std::get<1>(key), std::get<2>(key),
                 std::accumulate(runtimes.begin(), runtimes.end(), double{0.0}) / runtimes.size());
  }

  #ifdef SERVER
    char hostname[255];
    gethostname(hostname, sizeof(hostname));
    const auto hostname_string = std::string(hostname);
    const auto vector_size_in_gb = (sizeof(uint32_t) * VECTOR_SIZE) / 1000 / 1000 / 1000;
    const auto filename = std::format("plotting/prefetching/{}_{}GB.csv", hostname_string, vector_size_in_gb);
    auto output_stream = std::ofstream{filename};
    output_stream << "MACHINE,NAME,VECTOR_SIZE,LOCALITY,OFFSET,RUNTIME_S\n";
    for (const auto& [key, runtimes] : results) {
      const auto header = std::format("\"{}\",\"{}\",{},{},{},", hostname_string, std::get<0>(key), VECTOR_SIZE,
                                      std::get<1>(key), std::get<2>(key));
      for (const auto runtime : runtimes) {
        output_stream << header << runtime << "\n";
      }
    }
    output_stream.close();
  #endif

  return 0;
}
