//
//  CacheBenchmark
//
//  Created by Lawrence Benson on 27.04.22.
//  Adapted by Martin Boissier in July 2024.
//

#include <unistd.h>  // gethostname()

#include <algorithm>
#include <chrono>
#include <fstream>
#include <iostream>
#include <numeric>
#include <random>
#include <vector>

constexpr auto NUM_OPS = size_t{100'000'000};

auto sum = size_t{0};

size_t run_bm(size_t element_count) {
    auto data = std::vector<uint64_t>(element_count);

    auto random_device = std::random_device{};
    auto generator = std::mt19937{random_device()};

    std::iota(data.begin(), data.end(), 0);
    std::shuffle(data.begin(), data.end(), generator);

    // Do one pass to make sure everything has been touched.
    for (auto index = size_t{0}; index < element_count; ++index) {
        sum += data[index];
    }

    // Actually run the benchmark.
    const auto start = std::chrono::steady_clock::now();

    size_t next_position = size_t{0};
    for (auto index = size_t{0}; index < NUM_OPS; ++index) {
        next_position = data[next_position];
        sum += next_position;
    }

    const auto end = std::chrono::steady_clock::now();
    const auto duration = (end - start);

    return duration.count();
}

int main() {
    auto csv_partials = std::vector<std::string>{};
    auto run_count = size_t{1};

    #ifdef FULLRUN
        run_count = 5;
    #endif

    
    for (auto run_id = size_t{0}; run_id < run_count; ++run_id) {
        for (auto index = size_t{0}; index < 27; ++index) {  // 1 KB to 64 GB.
        //for (auto index = size_t{0}; index < 5; ++index) {
            const auto element_count = size_t{128} << index;
            const auto duration = run_bm(element_count);

            const auto ns_per_access = static_cast<double>(duration) / NUM_OPS;
            const auto bytes = element_count * sizeof(uint64_t);

            // < 1 MB --> use KB
            if (bytes < 1000 * 1000) {
                const auto kb = bytes / 1000;
                std::cout << kb << " KB";
            } else {
                const auto mb = bytes / 1024 / 1024;
                std::cout << mb << " MB";
            }

            std::cout << " took " << ns_per_access << " ns per access (" << element_count << " elements)" << std::endl;
 
            csv_partials.emplace_back(std::format("{},{},{},{}", run_id, element_count, bytes, duration));
        }
    }

    // Print sum to avoid optimization (might overflow).
    std::cout << "Sum = " << sum << std::endl;

    #ifdef FULLRUN
        char hostname[255];
        gethostname(hostname, sizeof(hostname));
        const auto hostname_string = std::string(hostname);
        const auto filename = std::format("plotting/pointer_chasing/{}.csv", hostname_string);
        auto output_stream = std::ofstream{filename};
        output_stream << "MACHINE,RUN_ID,ELEMENT_COUNT,SIZE_IN_BYTES,RUNTIME_NS\n";
        for (const auto& partial : csv_partials) {
          output_stream << std::format("\"{}\",{}\n", hostname_string, partial);
        }
        output_stream.close();
    #endif
}

