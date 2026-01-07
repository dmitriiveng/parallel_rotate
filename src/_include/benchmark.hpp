#pragma once

#include "rotate_parallel.hpp"

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

    {
        std::cout << "test\n";
        std::forward_list<int> v = {1,2,3,4,5,6,7,8,9,10};
        print_container(v);
        auto mid = v.begin();
        std::advance(mid, 7);
        rotate_forward_inplace(v.begin(), mid, v.end(), 2);
        print_container(v);
    }
}