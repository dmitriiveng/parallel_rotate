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
#include <boost/asio.hpp>

template <typename ForwardIt>
std::pair<ForwardIt, ForwardIt> swap_blocks_parallel(ForwardIt a, ForwardIt b, size_t length, size_t threadCount, boost::asio::thread_pool &pool) {
    if (length == 0) return std::pair<ForwardIt, ForwardIt>{a, b};

    //ограничиваем число потоков
    size_t maxThreads = std::max<size_t>(1, length / 1000);
    threadCount = std::min(threadCount, maxThreads);

    size_t baseSize = length / threadCount;
    size_t remainder = length % threadCount;

    ForwardIt aIt = a;
    ForwardIt bIt = b;

    std::pair<ForwardIt, ForwardIt> rv_end_iterators{};

    for (size_t i = 0; i < threadCount; ++i) {
        size_t blockSize = baseSize + (i < remainder ? 1 : 0);
        if (blockSize == 0) break;

        ForwardIt blockA = aIt;
        ForwardIt blockB = bIt;

        boost::asio::post(pool, [blockA, blockB, blockSize]() mutable {
            ForwardIt itA = blockA;
            ForwardIt itB = blockB;
            for (size_t j = 0; j < blockSize; ++j, ++itA, ++itB) {
                std::iter_swap(itA, itB);
            }
        });

        // сдвигаем итераторы на следующий блок (то, что за нас делал бы spliterator)
        for (size_t j = 0; j < blockSize; ++j) { ++aIt; ++bIt; }
        if(i == threadCount - 1){
            rv_end_iterators.first = aIt;
            rv_end_iterators.second = bIt;
        }
    }
    return rv_end_iterators;
}

template <typename ForwardIt>
ForwardIt rotate_forward_inplace(ForwardIt first, ForwardIt middle, ForwardIt last, size_t threadCount) {
    if (first == middle || middle == last) return first;

    boost::asio::thread_pool pool(threadCount);

    size_t leftLen = std::distance(first, middle);
    size_t rightLen = std::distance(middle, last);

    ForwardIt leftBegin = first;
    ForwardIt rightBegin = middle;

    while (leftLen > 0 && rightLen > 0) {
        if (leftLen <= rightLen) {
            // свапаем левый блок с первой частью правого блока той же длины

            std::pair<ForwardIt, ForwardIt> rv_end_iterators = 
                swap_blocks_parallel(leftBegin, rightBegin, leftLen, threadCount, pool);

            // после свапа левый блок сдвинулся вправо
            leftBegin = rightBegin;
            rightBegin = rv_end_iterators.second;
            rightLen -= leftLen;

        } else {
            // левый блок больше, свапаем правый блок с последней частью левого блока
            std::pair<ForwardIt, ForwardIt> rv_end_iterators = 
                swap_blocks_parallel(leftBegin, rightBegin, rightLen, threadCount, pool);

            // теперь оставшаяся часть левого блока
            leftLen -= rightLen;
            leftBegin = rv_end_iterators.first;
        }
    }

    pool.join();
    return leftBegin;
}


//benchmark helper
template <typename F>
long long time_ns(F&& f, size_t iterations = 1) {
    using clock = std::chrono::high_resolution_clock;
    std::vector<long long> times;
    times.reserve(iterations);

    for (size_t i = 0; i < iterations; ++i) {
        auto start = clock::now();
        f();
        auto end = clock::now();

        long long ns = static_cast<long long>(
            std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count()
        );
        times.push_back(ns);
    }

    // сортируем и берем медиану
    std::sort(times.begin(), times.end());
    if (times.empty()) return 0;

    if (times.size() % 2 == 1)
        return times[times.size() / 2];
    else
        return (times[times.size()/2 - 1] + times[times.size()/2]) / 2;
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

    //std::cout << "Size: " << data.size() << ", Middle: " << middle << "\n";
    std::cout << "Custom rotate (1 thread): " << t1 / 1e6 << " ms\n";
    std::cout << "Custom rotate (" << threads << " threads): " << tN / 1e6 << " ms\n";
    std::cout << "Speedup: " << double(t1) / double(tN) << "x\n";
    std::cout << "std::rotate: " << t_std / 1e6 << " ms\n";
    std::cout << "Speedup for std::rotate: " << double(t_std) / double(tN) << "x\n\n";
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
        std::cout << "test\n";
        std::forward_list<int> v = {1,2,3,4,5,6};
        print_container(v);
        auto mid = v.begin();
        std::advance(mid, 2);
        rotate_forward_inplace(v.begin(), mid, v.end(), 2);
        print_container(v);
    }

    {
        std::cout << "test\n";
        std::forward_list<int> v = {1,2,3,4,5};
        print_container(v);
        auto mid = v.begin();
        std::advance(mid, 2);
        rotate_forward_inplace(v.begin(), mid, v.end(), 2);
        print_container(v);
    }

    {
        std::cout << "test\n";
        std::forward_list<int> v = {1,2,3,4,5,6,7,8,9,10};
        print_container(v);
        auto mid = v.begin();
        std::advance(mid, 4);
        rotate_forward_inplace(v.begin(), mid, v.end(), 2);
        print_container(v);
    }
}

//main
int main() {
    std::cout << "==== Test Cases ====\n";
    test_cases();

    std::cout << "\n==== Benchmark ====\n";
    size_t hw = std::thread::hardware_concurrency();
    if (hw == 0) hw = 4;

    std::forward_list<int> data1(1'000);
    std::iota(data1.begin(), data1.end(), 0);
    benchmark_rotate_generic(data1, 430, hw, 30);

    std::forward_list<int> data2(10'000);
    std::iota(data2.begin(), data2.end(), 0);
    benchmark_rotate_generic(data2, 4'230, hw, 30);

    std::forward_list<int> data3(100'000);
    std::iota(data3.begin(), data3.end(), 0);
    benchmark_rotate_generic(data3, 53'334, hw, 30);

    std::forward_list<int> data4(10'000'000);
    std::iota(data4.begin(), data4.end(), 0);
    benchmark_rotate_generic(data4, 4'230'000, hw, 5);

    return 0;
}