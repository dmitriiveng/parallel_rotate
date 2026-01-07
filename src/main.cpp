#include "rotate_parallel.hpp"
#include "benchmark.hpp"

int main() {
    std::cout << "==== Test Cases ====\n";
    test_cases();

    std::cout << "\n==== Benchmark ====\n";
    size_t hw = std::thread::hardware_concurrency();
    if (hw == 0) hw = 4;

    std::forward_list<int> data1(1'000'000);
    std::iota(data1.begin(), data1.end(), 0);
    benchmark_rotate_generic(data1, 2'230, hw, 5);

    std::forward_list<int> data2(10'000'000);
    std::iota(data2.begin(), data2.end(), 0);
    benchmark_rotate_generic(data2, 2'000'000, hw, 5);

    std::forward_list<int> data3(100'000'000);
    std::iota(data3.begin(), data3.end(), 0);
    benchmark_rotate_generic(data3, 40'230'000, hw, 5);

    return 0;
}