//
//  predication.cpp
//  Predication example. Will not work with optimization flags.
//
//  Created by Tilmann Rabl on 02.05.22.
//

#include <iostream>

int main(int argc, const char * argv[]) {
    
    int size = 10000000;
    int *data = new int[size];
    int count = 0;
    int count1 = 0;
    int count2 = 0;
    int n = INT_MAX/2;
    
    for (int i = 0; i < size; i++) {
        data[i] = rand();
    }
    for (int i=0; i<size; i++) {
        count += (data[i] < n);
    }
    std::cout << "Sum: "<< count << "\n";
    
    count = 0;
    const auto startBranch = std::chrono::steady_clock::now();
    for (int i=0; i<size; i++) {
        if (data[i] < n)
            count++;
    }
    const auto endBranch = std::chrono::steady_clock::now();
    std::cout << "Duration branched: "<< std::chrono::duration_cast<std::chrono::microseconds>(endBranch - startBranch).count() << "us\n";
    std::cout << "Sum: "<< count << "\n";
    
    count = 0;
    
    const auto startPred = std::chrono::steady_clock::now();
    for (int i=0; i<size; i++) {
        count += (data[i] < n);
    }
    const auto endPred = std::chrono::steady_clock::now();
    std::cout << "Duration predicated: "<< std::chrono::duration_cast<std::chrono::microseconds>(endPred - startPred).count() << "us\n";
    std::cout << "Sum: "<< count << "\n";
    
    const auto ooostart = std::chrono::steady_clock::now();

    for (int i=0; i<(size/2); i++) {
        count1 += (data[i] < n);
        count2 += (data[i+(size/2)] < n);
    }
    count = count1+count2;

    const auto oooend = std::chrono::steady_clock::now();
    std::cout << "Duration out of order: "<< std::chrono::duration_cast<std::chrono::microseconds>(oooend - ooostart).count() << "us\n";
    
    std::cout << "Sum: "<< count << "\n";
 
    
    return 0;
}
