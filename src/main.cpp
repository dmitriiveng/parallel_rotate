#include "rotate_parallel.hpp"
#include "rotate_parallel++.hpp"
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

    /*
    benchmark_rotate_generic(build<std::forward_list<int>>(1'000'000), 1000, hw, 10);
    benchmark_rotate_generic(build<std::forward_list<int>>(1'000'000), 2000, hw, 10);
    benchmark_rotate_generic(build<std::forward_list<int>>(1'000'000), 3000, hw, 10);
    benchmark_rotate_generic(build<std::forward_list<int>>(1'000'000), 4000, hw, 10);
    benchmark_rotate_generic(build<std::forward_list<int>>(1'000'000), 5000, hw, 10);
    benchmark_rotate_generic(build<std::forward_list<int>>(1'000'000), 6000, hw, 10);
    benchmark_rotate_generic(build<std::forward_list<int>>(1'000'000), 7000, hw, 10);
    benchmark_rotate_generic(build<std::forward_list<int>>(1'000'000), 8000, hw, 10);
    benchmark_rotate_generic(build<std::forward_list<int>>(1'000'000), 9000, hw, 10);
    */

    benchmark_rotate_generic(build<std::forward_list<int>>(100'000'000), 1000, hw, 10);

    return 0;
}