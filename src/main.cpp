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

template <typename ForwardIt>
void swap_blocks_parallel(ForwardIt a, ForwardIt b, size_t length, size_t threadCount) {
    if (length == 0) return;
    //ограничиваем число потоков
    size_t maxThreads = std::max<size_t>(1, length / 1000);
    threadCount = std::min(threadCount, maxThreads);    
    //std::cout << "length " << length << ", maxThreads " << maxThreads << ", threadCount " << threadCount << std::endl;

    size_t baseSize = length / threadCount;
    size_t remainder = length % threadCount;

    std::vector<std::thread> threads;

    size_t offset = 0;
    for (size_t i = 0; i < threadCount; ++i) {
        size_t blockSize = baseSize + (i < remainder ? 1 : 0);
        if (blockSize == 0) break;

        threads.emplace_back([=]() mutable {
            ForwardIt aIt = a;
            ForwardIt bIt = b;
            // двигаем итераторы до начала своего блока
            for (size_t k = 0; k < offset; ++k) {
                ++aIt;
                ++bIt;
            }

            // теперь выполняем swap своего блока
            for (size_t j = 0; j < blockSize; ++j) {
                std::swap(*aIt, *bIt);
                ++aIt;
                ++bIt;
            }
        });

        offset += blockSize; // смещение для следующего блока
    }

    for (auto& t : threads) t.join();
}

// rotate_forward_inplace: parallel block-swap
template <typename ForwardIt>
ForwardIt rotate_forward_inplace(ForwardIt first, ForwardIt middle, ForwardIt last, size_t threadCount) {
    if (first == middle || middle == last) return first;

    // вычисляем длины блоков
    size_t leftLen = 0;
    for (ForwardIt it = first; it != middle; ++it) ++leftLen;

    size_t rightLen = 0;
    for (ForwardIt it = middle; it != last; ++it) ++rightLen;

    ForwardIt leftBegin = first;
    ForwardIt rightBegin = middle;

    while (leftLen > 0 && rightLen > 0) {
        if /*находим меньший блок*/ (leftLen <= rightLen) {
            // swap leftLen элементов из left с началом right
            ForwardIt rightPart = rightBegin;
            for (size_t i = 0; i < leftLen; ++i) ++rightPart;

            swap_blocks_parallel(leftBegin, rightBegin, leftLen, threadCount);

            leftBegin = rightBegin;
            rightBegin = rightPart;
            rightLen -= leftLen;
        } else {
            // swap конец left с началом right
            ForwardIt leftPart = leftBegin;
            for (size_t i = 0; i < leftLen - rightLen; ++i) ++leftPart;

            swap_blocks_parallel(leftPart, rightBegin, rightLen, threadCount);

            leftLen -= rightLen;
        }
    }

    return leftBegin; // так же, как std::rotate возвращает новый "first"
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
        auto it = c.begin();
        std::advance(it, middle);
        rotate_forward_inplace(c.begin(), it, c.end(), t);
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
    std::cout << "Speedup for std::rotate: " << double(t_std) / double(tN) << "x\n";
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
        auto mid = v.begin();
        std::advance(mid, 2);
        rotate_forward_inplace(v.begin(), mid, v.end(), 2);
        print_container(v); // 3 4 5 6 1 2
    }
}

//main
int main() {
    std::cout << "==== Test Cases ====\n";
    test_cases();

    std::cout << "\n==== Benchmark (vector) ====\n";
    size_t hw = std::thread::hardware_concurrency();
    if (hw == 0) hw = 4;

    std::vector<int> data1(1'000'000);
    std::iota(data1.begin(), data1.end(), 0);
    benchmark_rotate_generic(data1, 434'230, hw, 3);

    std::vector<int> data2(2'000'000'000);
    std::iota(data2.begin(), data2.end(), 0);
    benchmark_rotate_generic(data2, 533'324'500, hw, 1);

    return 0;
}