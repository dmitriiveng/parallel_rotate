#pragma once

#include <forward_list>
#include <vector>
#include <cassert>
#include <thread>
#include <latch>
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
#include <limits>
#include <chrono>
#include <boost/asio.hpp>
#include <semaphore>

#include "block_swap.hpp"

template <typename ForwardIt>
void print_range(ForwardIt first, ForwardIt last, const char* prefix = "") {
    std::cout << prefix;
    bool first_elem = true;
    for (; first != last; ++first) {
        if (!first_elem) std::cout << ' ';
        first_elem = false;
        std::cout << *first;
    }
    std::cout << std::endl;
}

template <typename ForwardIt>
void rotate_first_block_forward_in_vector(
    std::vector<ForwardIt> v, 
    size_t block_length,
    size_t threadCount, 
    size_t MIN_PARALLEL_LENGTH,
    boost::asio::thread_pool &pool
){
    //ну тут все простенько, можно и лучше но оно и так должно быстро работать и ТЗ не нарушать
    if (v.size() < 2) return;

    for (size_t i = 0; i + 1 < v.size(); i++) {
        block_swap_parallel(
            v[i], v[i + 1],
            block_length,
            threadCount,
            MIN_PARALLEL_LENGTH,
            pool
        );
    }
}

template <typename ForwardIt>
std::pair<ForwardIt, ForwardIt> right_rotate_aliquot(//работает только если length_a < length_b
    ForwardIt it_first, ForwardIt it_second,
    size_t length_a, size_t length_b,
    size_t threadCount, size_t MIN_PARALLEL_LENGTH,
    boost::asio::thread_pool &pool
) {
    assert(length_a < length_b);
    using BlockIt = ForwardIt;

    ForwardIt it_a = it_first;
    ForwardIt it_b = it_second;

    size_t block_count = length_b / length_a + 1; //кол-во полных блоков (не считая начало)
    size_t remainder = length_b % length_a; //остаток
    size_t block_size = length_a; //размер одного блока

    //минимально должно обрабатываться по 2 блока в потоке
    size_t min_block_chunk_count = block_count / 2;
    //учитываем, что сами блоки разной длины
    size_t maxThreads = std::max<size_t>(min_block_chunk_count, block_count / threadCount);
    //ограничиваем число потоков
    size_t current_thread_count = std::min(threadCount, maxThreads);

    //размер 1 чанка в блоках
    size_t block_chunk_size = block_count / current_thread_count;
    size_t block_chunk_remainder = block_count % current_thread_count;

    //ветктор для свапнутых вправо блоков в каждом потоке O(кол-во потоков) не O(N)
    std::vector<BlockIt> second_level;
    second_level.reserve(current_thread_count);

    std::latch done(current_thread_count);

    // заполняем текущий уровень
    for (size_t i = 0; i < current_thread_count; ++i) {
        size_t current_block_chunk_size = block_chunk_size + (i < block_chunk_remainder ? 1 : 0);
        if (current_block_chunk_size == 0) break;

        ForwardIt blockStartA = it_a;
        ForwardIt blockStartB = it_b;

        boost::asio::post(pool, [blockStartA, blockStartB, current_block_chunk_size, block_size, &done]() mutable {
            ForwardIt itA = blockStartA;
            ForwardIt itB = blockStartB;
            for (size_t j = 0; j < current_block_chunk_size - 1; ++j) {
                block_swap(itA, itB, block_size);
                std::advance(itA, block_size);
                std::advance(itB, block_size);
            }
            done.count_down();
        });

        // сдвигаем итераторы на следующий блок и записывает сдвинутый блок в след уровень
        std::advance(it_a, (current_block_chunk_size - 1) * block_size);
        std::advance(it_b, (current_block_chunk_size - 1) * block_size);
        second_level.push_back(it_a);

        if(i < current_thread_count - 1) {
            std::advance(it_b, block_size);
            std::advance(it_a, block_size);
        }
    }

    //ждем пока все отработает
    done.wait();

    //тут все O(кол-во потоков), так что можно себе многое позволить
    rotate_first_block_forward_in_vector(
        second_level, 
        block_size,
        threadCount, 
        MIN_PARALLEL_LENGTH,
        pool
    );

    // Возврат диапазона
    return {it_a, it_b};
}
