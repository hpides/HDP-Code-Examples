//
//  MemorySortBenchmark.cpp
//
//  Created by Tilmann Rabl on 25.04.22. Extended by Martin Boissier in 2024.
//

#include <algorithm>
#include <chrono>
#include <execution>
#include <iostream>
#include <vector>

int main(int argc, char * argv[]) {
    #ifdef PARALLEL_STD_SORT
    std::cout << "Running multi-threaded std::sort on ";
    constexpr auto is_single_threaded = false;
    #else
    std::cout << "Running single-threaded std::sort on ";
    constexpr auto is_single_threaded = true;
    #endif

    #ifdef LARGE_DATASET
    std::cout << "16 GB array.\n";
    const auto item_count = uint64_t{4'000'000'000};
    //const auto item_count = uint64_t{40'000'000};
    #else
    std::cout << "1 GB array.\n";
    const auto item_count = uint64_t{250'000'000};
    //const auto item_count = uint64_t{2'500'000};
    #endif

    auto data = std::vector<int32_t>(item_count, -1);

    // Output array sizes and initial first 10 elements
    std::cout << "Element size (int32_t): " << sizeof(int32_t) << " Bytes\n";
    std::cout << "Total array size: " << sizeof(int32_t) * item_count / 1000 / 1000 << " MBytes\n";

    // Fill array with random numbers. If we use a smaller domain with modulo, we can observe the
    // IntroSort using Insertion Sort.
    std::cout << "Filling array with random data ..." << std::flush;
    for (auto i = uint64_t{0}; i < item_count; i++) {
        data[i] = rand(); // % 10000;
    }
    std::cout << " done\n";

    std::cout << "Initial array start: ";
    for (auto i = uint8_t{0}; i < 10; i++) {
        std::cout << data[i] << " ";
    }
    std::cout << "\n";

    if (argc > 1) {
       if (std::string{argv[1]} != "STOP" || argc > 2) {
         std::cerr << "Please pass either 'STOP' as single argument or nothing.\n";
         std::exit(17);
       }
       std::cout << "Early out for measuring data creation only.\n";
       std::exit(17);
    }

    // Start measurement
    const auto start = std::chrono::steady_clock::now();

    // Actual sorting using std::sort
    if (is_single_threaded) {
        std::sort(std::execution::seq, data.begin(), data.end());
    } else {
        std::sort(std::execution::par_unseq, data.begin(), data.end());
    }

    // End measurement
    const auto end = std::chrono::steady_clock::now();
    const auto duration = std::chrono::duration<double>(end - start);

    // Output first 10 elements of the array after sorting
    std::cout << "Array start after sorting: ";

    for (auto i = uint8_t{0}; i < 10; i++) {
        std::cout << data[i] << " ";
    }
    std::cout << "\n";

    // Output total duration in ms
    std::cout << "Total duration: " << duration.count() << " s\n";

    return 0;
}