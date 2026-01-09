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

    auto run_custom_sb = [&](size_t t) {
        Container c = data;
        auto it = c.begin();
        std::advance(it, middle);
        rotate_forward_inplace_swap_blocks(c.begin(), it, c.end(), t);
    };

    auto run_custom_lm = [&](size_t t) {
        Container c = data;
        auto it = c.begin();
        std::advance(it, middle);
        rotate_forward_inplace_low_middle(c.begin(), it, c.end(), t);
    };

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

    long long t_single = time_ns([&]() { run_custom(1); }, repeats);
    std::cout << "Custom rotate (1 thread): " << t_single / 1e6 << " ms\n";
    long long t = time_ns([&]() { run_custom(threads); }, repeats);
    std::cout << "Custom rotate (" << threads << " threads): " << t / 1e6 << " ms\n";
    long long t_lm = time_ns([&]() { run_custom_lm(threads); }, repeats);
    std::cout << "Custom rotate low middle only (" << threads << " threads): " << t_lm / 1e6 << " ms\n";
    long long t_sb = time_ns([&]() { run_custom_sb(threads); }, repeats);
    std::cout << "Custom rotate swap blocks only (" << threads << " threads): " << t_sb / 1e6 << " ms\n";
    long long t_std = time_ns([&]() { run_std(); }, repeats);
    std::cout << "std::rotate: " << t_std / 1e6 << " ms\n\n";
    std::cout << "speedup to single: " << double(t) / double(t_single) << "x\n";
    std::cout << "speedup to std::rotate: " << double(t) / double(t_std) << "x\n";
    std::cout << "speedup low middle to std::rotate: " << double(t_std) / double(t_lm) << "x\n";
    std::cout << "speedup swap blocks to std::rotate: " << double(t_std) / double(t_sb) << "x\n\n\n";
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
        std::cout << "test_low_middle\n";
        std::forward_list<int> v = {1,2,3,4,5};
        print_container(v);
        auto mid = v.begin();
        std::advance(mid, 2);
        rotate_forward_inplace_low_middle(v.begin(), mid, v.end(), 2);
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

    {
        std::cout << "test_low_middle\n";
        std::forward_list<int> v = {1,2,3,4,5,6,7,8,9,10};
        print_container(v);
        auto mid = v.begin();
        std::advance(mid, 7);
        rotate_forward_inplace_low_middle(v.begin(), mid, v.end(), 2);
        print_container(v);
    }

    {
        std::cout << "test\n";
        std::forward_list<int> v = {1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20};
        print_container(v);
        auto mid = v.begin();
        std::advance(mid, 11);
        rotate_forward_inplace(v.begin(), mid, v.end(), 2);
        print_container(v);

        std::forward_list<int> v2 = {1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20};
        auto mid2 = v2.begin();
        std::advance(mid2, 11);
        std::rotate(v2.begin(), mid2, v2.end());

        int e = static_cast<int>(v == v2);
        print_container(v2);

        std::cout << "same as std::rotate: " << e << std::endl;
    }
}