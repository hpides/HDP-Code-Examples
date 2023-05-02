//
//  main.cpp
//  MemorySortBenchmark
//
//  Created by Tilmann Rabl on 25.04.22.
//

#include <iostream>
#include <algorithm>
#include <vector>
#include <chrono>

int main(int argc, const char * argv[]) {
    
    int size = 268435456;
    std::vector<int> data;
    data.reserve(size);
    uint64_t dur = 0;
  
    // Fill array with random numbers
    // If we use a smaller domain with modulo, we can observe the
    // IntroSort using Insertion Sort
    for (int i = 0; i< size; i++) {
        data.emplace_back(rand()); // % 10000;
    }

    // Output array sizes and initial first 10 elements
    std::cout << "Int size: " << sizeof(int) << " Bytes\n";
    std::cout << "Total array size: " << sizeof(int)*size/1024/1024 << " MBytes\n";
    std::cout << "Initial array start: ";
    for (int i = 0; i < 10; i++) {
        std::cout << data[i] << " ";
    }
    std::cout << "\n";
    
    // Start measurement
    const auto start = std::chrono::steady_clock::now();
    
    // Actual sorting using std::sort
    std::sort(data.begin(), data.end());
    
    // End measurement
    const auto end = std::chrono::steady_clock::now();
    dur += std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();

    // Output first 10 elements of the array after sorting
    std::cout << "Array start after sort: ";
    
    for (int i = 0; i < 10; i++) {
        std::cout << data[i] << " ";
    }
    std::cout << "\n";
    
    // Output total duration in ms
    std::cout << "Total duration: " << dur/1000 << "ms\n";
    return 0;
    
}
