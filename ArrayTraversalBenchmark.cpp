//
//  ArrayTraversalBenchmark.cpp
//  Test Project
//
//  Created by Tilmann Rabl on 11.03.22.
//

#include <iostream>
#include <cmath>
#include <chrono>

int main(int argc, const char * argv[]) {
    
    // Initialize array, sizes, and timers
    int size = 1000000000;
    int rows = 10000;
    int cols = 10000;
    int *src = new int[size];
    int iterations = 8;
    uint64_t *dur_row_wise = new uint64_t[8];
    uint64_t *dur_col_wise = new uint64_t[8];
    uint64_t sum = 0;
    
    // Fill array with random numbers between 0 and 10
    for (int i = 0; i< size; i++) {
        src[i] = rand() % 10;
    }
    
    // One initial run to ensure everything is
    // in caches for the other runs
    sum=0;
    for (int r = 0; r < rows; r++)
        for (int c = 0; c < cols; c++)
            sum += src[r * cols + c];
    
    
    for (int run = 0 ; run < iterations; run++) {
        // Calculate number of rows and columns such that we always
        // have #size elements
        rows = pow(10,run+1);
        cols = size / rows;
        std::cout << "Columns: " << cols << " Rows: " << rows << "\n";
        
        sum=0;
        
        // Benchmark column-wise traversal
        const auto start_col_wise = std::chrono::steady_clock::now();
        for (int c = 0; c < cols; c++)
            for (int r = 0; r < rows; r++)
                sum += src[r * cols + c];
        const auto end_col_wise = std::chrono::steady_clock::now();

        dur_col_wise[run] = std::chrono::duration_cast<std::chrono::microseconds>(end_col_wise - start_col_wise).count();
        
        std::cout << " col-wise:" << dur_col_wise[run]/1000 << " ms sum:" << sum << " ";
        
        sum = 0;

        // Benchmark row-wise traversal
        const auto start_row_wise = std::chrono::steady_clock::now();
        
        for (int r = 0; r < rows; r++)
            for (int c = 0; c < cols; c++)
                sum += src[r * cols + c];
        
        const auto end_row_wise = std::chrono::steady_clock::now();
        dur_row_wise[run] = std::chrono::duration_cast<std::chrono::microseconds>(end_row_wise - start_row_wise).count();
        
        std::cout << " row-wise:" << dur_row_wise[run]/1000 << " ms sum:" << sum << "\n";
    }
    
    // Output results
    std::cout << "Sum: " << sum << "\n";
    std::cout << "Total duration row-wise (ms): \n";
    for (int i = 0; i < iterations ; i++)
        std::cout << dur_row_wise[i]/1000 << "\t";
    std::cout << "\n";
    std::cout << "Total duration column-wise: \n";
    for (int i = 0; i < iterations ; i++)
        std::cout << dur_col_wise[i]/1000 << "\t";
    std::cout << "\n";
    
    return 0;
    
    
}


