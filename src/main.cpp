#include "rotate_parallel.hpp"
#include "benchmark.hpp"

template <typename Container>
Container build(size_t len) {
    Container data(len);
    std::iota(data.begin(), data.end(), 0);
    return data;
}

int main() {
    size_t hw = std::thread::hardware_concurrency();
    if (hw == 0) hw = 4;

    std::cout << "==== Test Cases ====\n";
    //test_cases();

    test_rotate_generic(build<std::forward_list<int>>(50), 12, hw, 1);

    test_rotate_generic(build<std::forward_list<int>>(1'000'000), 12'432, hw, 1);
    test_rotate_generic(build<std::forward_list<int>>(1'432'213), 432'423, hw, 1);
    test_rotate_generic(build<std::forward_list<int>>(1'000'000), 500'001, hw, 1);
    test_rotate_generic(build<std::forward_list<int>>(1'112'423), 999'231, hw, 1);
    test_rotate_generic(build<std::forward_list<int>>(1'000'000), 1, hw, 1);
    test_rotate_generic(build<std::forward_list<int>>(1'000'000), 999'999, hw, 1);

    std::cout << "\n==== Benchmark ====\n";

    benchmark_rotate_generic(build<std::forward_list<int>>(10'000'000), 12'432, hw, 10);
    benchmark_rotate_generic(build<std::forward_list<int>>(100'000'000), 1'432'432, hw, 3);
    benchmark_rotate_generic(build<std::forward_list<int>>(10'000'000), 6'232'432, hw, 10);
    benchmark_rotate_generic(build<std::forward_list<int>>(100'000'000), 62'432'432, hw, 3);

    return 0;
}