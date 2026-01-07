#include <forward_list>
#include <iostream>
#include <vector>
#include <cassert>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <functional>
#include <cassert>
#include <limits>
#include <chrono>
#include <algorithm>
#include <numeric>
#include <iostream>
#include <vector>
#include <forward_list>
#include <thread>
#include <algorithm>
#include <numeric>
#include <limits>
#include <chrono>

// swap_blocks_parallel
// по сути многопоточный swap_ranges
template <typename ForwardIt>
void swap_blocks_parallel(ForwardIt a, ForwardIt b, size_t length, size_t threadCount) {
    if (length == 0) return;
    threadCount = std::min(threadCount, length);

    //расчет размера чанков
    size_t baseSize = length / threadCount;
    size_t remainder = length % threadCount;

    //пулл потоков
    std::vector<std::thread> threads;
    ForwardIt blockA = a;
    ForwardIt blockB = b;

    for (size_t i = 0; i < threadCount; ++i) {
        //вычисление длинны отдельного блока (чанка)
        size_t blockSize = baseSize + (i < remainder ? 1 : 0);
        if (blockSize == 0) break;

        ForwardIt aIt = blockA;
        ForwardIt bIt = blockB;

        threads.emplace_back([aIt, bIt, blockSize]() mutable {
            for (size_t j = 0; j < blockSize; ++j) {
                std::swap(*aIt, *bIt);
                ++aIt;
                ++bIt;
            }
        });

        //сдвигаем два следующих итаратора на нужное число элементов
        for (size_t j = 0; j < blockSize; ++j) {
            ++blockA;
            ++blockB;
        }
    }

    for (auto& t : threads) t.join();
}

// rotate_forward_inplace
template <typename ForwardIt>
void rotate_forward_inplace(ForwardIt begin, size_t totalLength, size_t middleIndex, size_t threadCount) {
    if (middleIndex == 0 || middleIndex == totalLength) return;

    ForwardIt leftBegin = begin;
    size_t leftLen = middleIndex;

    ForwardIt rightBegin = begin;
    for (size_t i = 0; i < middleIndex; ++i) ++rightBegin;
    size_t rightLen = totalLength - middleIndex;

    while (leftLen > 0 && rightLen > 0) {
        if (leftLen <= rightLen) {
            ForwardIt rightPart = rightBegin;
            for (size_t i = 0; i < leftLen; ++i) ++rightPart;

            swap_blocks_parallel(leftBegin, rightBegin, leftLen, threadCount);

            leftBegin = rightBegin;
            rightBegin = rightPart;
            rightLen -= leftLen;
        } else {
            ForwardIt leftPart = leftBegin;
            for (size_t i = 0; i < leftLen - rightLen; ++i) ++leftPart;

            swap_blocks_parallel(leftPart, rightBegin, rightLen, threadCount);

            leftLen -= rightLen;
        }
    }
}

//benchmark helper
template <typename F>
long long time_ns(F&& f, size_t iterations = 1) {
    using clock = std::chrono::high_resolution_clock;
    long long best = std::numeric_limits<long long>::max();
    for (size_t i = 0; i < iterations; ++i) {
        auto start = clock::now();
        f();
        auto end = clock::now();
        long long ns = static_cast<long long>(
            std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count()
        );
        best = std::min(best, ns);
    }
    return best;
}

//benchmark

template <typename Container>
void benchmark_rotate_generic(const Container& data, size_t middle, size_t threads, size_t repeats) {
    auto run_custom = [&](size_t t) {
        Container c = data;
        rotate_forward_inplace(c.begin(), c.size(), middle, t);
    };

    auto run_std = [&]() {
        Container c = data;
        auto it = c.begin();
        std::advance(it, middle);
        std::rotate(c.begin(), it, c.end());
    };

    long long t1 = time_ns([&]() { run_custom(1); }, repeats);
    long long tN = time_ns([&]() { run_custom(threads); }, repeats);
    long long t_std = time_ns([&]() { run_std(); }, repeats);

    std::cout << "Size: " << data.size() << ", Middle: " << middle << "\n";
    std::cout << "Custom rotate (1 thread): " << t1 / 1e6 << " ms\n";
    std::cout << "Custom rotate (" << threads << " threads): " << tN / 1e6 << " ms\n";
    std::cout << "Speedup: " << double(t1) / double(tN) << "x\n";
    std::cout << "std::rotate: " << t_std / 1e6 << " ms\n\n";
}


//helper functions
template <typename T>
std::forward_list<T> make_forward_list(const std::vector<T>& v) {
    std::forward_list<T> lst;
    for (auto it = v.rbegin(); it != v.rend(); ++it) {
        lst.push_front(*it);
    }
    return lst;
}

template <typename T>
void print_container(const T& c) {
    for (const auto& x : c) std::cout << x << " ";
    std::cout << "\n";
}

//tests
void test_cases() {
    {
        std::vector<int> v = {1,2,3,4,5,6};
        rotate_forward_inplace(v.begin(), v.size(), 2, 2);
        print_container(v); // 3 4 5 6 1 2
    }
}

//main
int main() {
    std::cout << "==== Test Cases ====\n";
    test_cases();

    std::cout << "\n==== Benchmark (vector) ====\n";
    size_t hw = std::thread::hardware_concurrency();
    if (hw == 0) hw = 4; // fallback

    std::vector<int> data1(1'000'000);
    std::iota(data1.begin(), data1.end(), 0);
    benchmark_rotate_generic(data1, 456'789, hw, 3);

    std::vector<int> data2(1000000000);
    std::iota(data2.begin(), data2.end(), 0);
    benchmark_rotate_generic(data2, 400'345'678, hw, 2);

    return 0;
}