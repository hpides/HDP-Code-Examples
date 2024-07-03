//
//  main.cpp
//  MemorySortBenchmark
//
//  Created by Tilmann Rabl on 25.04.22. Extended by Martin Boissier in June 2024.
//

#include <algorithm>
#include <chrono>
#include <execution>
#include <iostream>
#include <vector>

int main(int /* argc */, const char ** /* argv[] */) {
<<<<<<< Updated upstream
    auto modes = std::vector<std::pair<std::string, bool>>{{"single-threaded", true}};

    #ifdef LINUXSERVER
    modes.emplace_back("multi-threaded", false);
    #endif

    auto size = uint64_t{4'294'967'296};  // 2^32
    auto data = std::vector<int32_t>(size, -1);

    // Output array sizes and initial first 10 elements.
    std::cout << "Element size (int32_t): " << sizeof(int32_t) << " Bytes\n";
    std::cout << "Total array size: " << sizeof(int32_t)*size/1000/1000 << " MBytes\n";

    for (const auto& [benchmark_name, is_single_threaded] : modes) {
        // Fill array with random numbers.
        // If we use a smaller domain with modulo, we can observe the
        // IntroSort using Insertion Sort.
        std::cout << "Filling array with random data ..." << std::flush;
        for (auto i = uint64_t{0}; i < size; i++) {
            data[i] = rand(); // % 10000;
        }
        std::cout << " done\n" << std::flush;

        std::cout << "Initial array start: ";
        for (auto i = uint64_t{0}; i < 10; i++) {
            std::cout << data[i] << " ";
        }
        std::cout << "\n";

        // Start measurement.
        const auto start = std::chrono::steady_clock::now();

        // Actual sorting using std::sort.
        if (is_single_threaded) {
            std::sort(std::execution::seq, data.begin(), data.end());
        } else {
            std::sort(std::execution::par_unseq, data.begin(), data.end());
        }

        // End measurement.
        const auto end = std::chrono::steady_clock::now();
        const auto duration = std::chrono::duration<double>(end - start);

        // Output first 10 elements of the array after sorting.
        std::cout << "Array start after sort: ";
        for (auto i = uint64_t{0}; i < 10; i++) {
            std::cout << data[i] << " ";
        }
        std::cout << "\n";

        // Output total duration in s.
        std::cout << "Total duration: " << duration.count() << " s\n";
    }

=======
    auto modes = std::vector<std::pair<std::string, bool>>{};

    #ifdef LARGER_PARALLEL
    std::cout << "Running multi-threaded std::sort in 16 GB array.\n";
    modes.emplace_back("multi-threaded", false);
    #else
    modes.emplace_back("single-threaded", true);
    #endif

    //auto size = uint64_t{4'294'967'296};  // 2^32
    auto size = uint64_t{294'967'296};  // 2^32
    auto data = std::vector<int32_t>(size, -1);

    // Output array sizes and initial first 10 elements
    std::cout << "Element size (int32_t): " << sizeof(int32_t) << " Bytes\n";
    std::cout << "Total array size: " << sizeof(int32_t) * size / 1000 / 1000 << " MBytes\n";

    for (const auto& [benchmark_name, is_single_threaded] : modes) {
        // Fill array with random numbers.
        // If we use a smaller domain with modulo, we can observe the
        // IntroSort using Insertion Sort.
        std::cout << "Filling array with random data ..." << std::flush;
        for (auto i = uint64_t{0}; i < size; i++) {
            data[i] = rand(); // % 10000;
        }
        std::cout << " done\n" << std::flush;

        std::cout << "Initial array start: ";
        for (auto i = uint64_t{0}; i < 10; i++) {
            std::cout << data[i] << " ";
        }
        std::cout << "\n";
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
        std::cout << "Array start after sort: ";

        for (auto i = uint64_t{0}; i < 10; i++) {
            std::cout << data[i] << " ";
        }
        std::cout << "\n";

        // Output total duration in ms
        std::cout << "Total duration: " << duration.count() << " s\n";
    }

>>>>>>> Stashed changes
    return 0;
}