#include <chrono>
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

// We use a string length that is long enough to spread struct fields over multiple cache lines.
// Most modern Intel/AMD servers have 64 byte cache lines while Apples M2 has 128 byte cache lines.
#if defined(__x86_64__) || defined(_M_X64)
constexpr auto STRING_LENGTH = size_t{28};
#elif defined(__aarch64__) || defined(_M_ARM64)
constexpr auto STRING_LENGTH = size_t{48};
#else
std::cerr << "Unexpected CPU.\n";
std::exit(EXIT_FAILURE);
#endif

struct S1 {
  int primary_key;
  long timestamp;
  char color[2];
  int zipcode;
};

#pragma pack(1)
struct S2 {
  int primary_key;
  long timestamp;
  char color[2];
  int zipcode;
};

#pragma pack(8)
struct S3 {
  int primary_key;
  long timestamp;
  char color[2];
  int zipcode;
};

#pragma pack(push, 1)
struct S4 {
  char color[2];
  long timestamp;
  int primary_key;
  int zipcode;
};
#pragma pack(pop)


// When packed, `amount_is_null` and `amount` use 9 bytes. To force `tax_rate` on the next cache line, we use two
// strings of size 28 (64 - 9 = 55; 55 / 2 = 28) on x86_64 and 60 on ARM64 platforms.
struct Sale1 {
  bool amount_is_null;
  double amount;
  char product_name[STRING_LENGTH];
  char product_category[STRING_LENGTH];
  bool taxes_incurred;
  double tax_rate;
};

#pragma pack(push, 1)
struct Sale2 {  // Pack.
  bool amount_is_null;
  double amount;
  char product_name[STRING_LENGTH];
  char product_category[STRING_LENGTH];
  bool taxes_incurred;
  double tax_rate;
};
#pragma pack(pop)

struct Sale3 {
  bool amount_is_null;
  double amount;
  bool taxes_incurred;
  double tax_rate;
  char product_name[STRING_LENGTH];
  char product_category[STRING_LENGTH];
};

struct Sale4 {
  bool amount_is_null;
  bool taxes_incurred;
  double amount;
  double tax_rate;
  char product_name[STRING_LENGTH];
  char product_category[STRING_LENGTH];
};

struct Sale5 {
  bool amount_is_null;
  bool taxes_incurred;
  double amount;
  double tax_rate;
  char product_name[STRING_LENGTH];
  char product_category[STRING_LENGTH];
  char _pad[8];
};

template <typename T>
void benchmark() {
  std::cout << "Size of single struct " << typeid(T).name() << " is " << sizeof(T) << " (alignof: " << alignof(T) << ").\n";

  // for (const auto size_factor : std::initializer_list<size_t>{8, 16, 32, 64}) {
  for (const auto size_factor : std::initializer_list<size_t>{32}) {
    const auto element_count = 1024 * 1024 * size_factor;

    auto struct_vector = std::vector<T>(element_count);

    auto start = std::chrono::steady_clock::now();
    for (auto i = size_t{0}; i < element_count; ++i) {
      if constexpr (std::is_same<T, Sale1>::value || std::is_same<T, Sale2>::value || std::is_same<T, Sale3>::value || std::is_same<T, Sale4>::value || std::is_same<T, Sale5>::value) {
        struct_vector[i] = T{.amount_is_null = i % 17 == 0, .amount = 10.0 * (i % 1'000),
                             .taxes_incurred = i % 117 == 0, .tax_rate = 1.01 * (i % 10)};
      } else {
        struct_vector[i] = T{};
      }
    }
    auto end = std::chrono::steady_clock::now();

    const auto data_size = sizeof(T)*element_count / 1000 / 1000;
    auto runtime_s = std::chrono::duration<double>(end - start);

    std::cout << "Filled with " << struct_vector.size() << " elements (" << data_size 
              << " MB) in " << runtime_s * 1000 << " ms ("
              << data_size / runtime_s.count() << " MB/s).\n";

    if constexpr (std::is_same<T, Sale1>::value || std::is_same<T, Sale2>::value || std::is_same<T, Sale3>::value || std::is_same<T, Sale4>::value || std::is_same<T, Sale5>::value) {
      start = std::chrono::steady_clock::now();
      auto sum = double{0.0};
      for (auto i = size_t{0}; i < element_count; ++i) {
        sum += (struct_vector[i].amount_is_null * struct_vector[i].amount) *
               (
                (struct_vector[i].taxes_incurred * struct_vector[i].tax_rate) + (1 - struct_vector[i].taxes_incurred)
               );

        // auto amount = 0.0;
        // if (!struct_vector[i].amount_is_null) {
        //   amount = struct_vector[i].amount;

        //   if (struct_vector[i].taxes_incurred) {
        //     amount *= struct_vector[i].tax_rate;
        //   }
        //   sum += amount;
        // }
      }
      end = std::chrono::steady_clock::now();

      runtime_s = std::chrono::duration<double>(end - start);
      std::cout << "Summed " << struct_vector.size() << " elements (result: " << sum << ", " << data_size 
                << " MB) in " << runtime_s * 1000 << " ms ("
                << data_size / runtime_s.count() << " MB/s).\n";
    }
  }
}

int main() {
  // benchmark<S1>();
  // benchmark<S2>();
  // benchmark<S3>();
  // benchmark<S4>();
  benchmark<Sale1>();
  benchmark<Sale2>();
  benchmark<Sale3>();
  benchmark<Sale4>();
  benchmark<Sale5>();
  // benchmark<S1>();
  // benchmark<S2>();
  // benchmark<S3>();
  // benchmark<S4>();
  benchmark<Sale1>();
  benchmark<Sale2>();
  benchmark<Sale3>();
  benchmark<Sale4>();
  benchmark<Sale5>();
  benchmark<Sale1>();
  benchmark<Sale2>();
  benchmark<Sale3>();
  benchmark<Sale4>();
  benchmark<Sale5>();

  return 0;
}
