#include "rotate_parallel.hpp"
#include "benchmark.hpp"

template <typename Container>
Container build(size_t len) {
    Container data(len);
    std::iota(data.begin(), data.end(), 0);
    return data;
}

int main() {
    std::cout << "==== Test Cases ====\n";
    test_cases();

    std::cout << "\n==== Benchmark ====\n";
    size_t hw = std::thread::hardware_concurrency();
    if (hw == 0) hw = 4;

    benchmark_rotate_generic(build<std::forward_list<int>>(10'000'000), 12432, hw, 10);

    return 0;
}