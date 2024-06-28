//
//  main.cpp
//  CacheBenchmark
//
//  Created by Lawrence Benson on 27.04.22.
//

#include <vector>
#include <numeric>
#include <random>
#include <iostream>
#include <chrono>
#include <algorithm>


constexpr size_t NUM_OPS = 100'000'000;

size_t sum = 0;

void run_bm(size_t n) {
    std::vector<uint64_t> data(n);
    
    std::random_device dev;
    std::mt19937 rng{dev()};
    
    std::iota(data.begin(), data.end(), 0);
    std::shuffle(data.begin(), data.end(), rng);

    // Do one pass to make sure everything has been touched.
    for (size_t i = 0; i < n; ++i) {
        sum += data[i];
    }
            
    // Actually run the benchmark
    auto start = std::chrono::steady_clock::now();
    
    size_t next_pos = 0;
    for (size_t i = 0; i < NUM_OPS; ++i) {
        next_pos = data[next_pos];
        sum += next_pos;
    }

    auto end = std::chrono::steady_clock::now();
    auto duration = (end - start);
    size_t ns_per_access = duration.count() / NUM_OPS;
    size_t bytes = n * sizeof(uint64_t);
    
    // < 1 MiB --> use KiB
    if (bytes < 1024 * 1024) {
        size_t kib = bytes / 1024;
        std::cout << kib << " KiB";
    } else {
        size_t mib = bytes / 1024 / 1024;
        std::cout << mib << " MiB";
    }
        
    std::cout << " took " << ns_per_access << " ns per access (n=" << n << ")" << std::endl;
}


int main() {
    // 1 KiB to 1 GiB
    for (size_t i = 0; i < 24; ++i) {
        size_t n = 128 << i;
        run_bm(n);
    }
    
    // Print sum to avoid optimization.
    std::cout << "Sum = " << sum << std::endl;
}


