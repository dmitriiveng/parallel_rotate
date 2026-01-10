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
void rotate_first_block_backward_in_vector(
    std::vector<ForwardIt> v, 
    size_t block_length,
    size_t threadCount, 
    size_t MIN_PARALLEL_LENGTH,
    boost::asio::thread_pool &pool
){
    //ну тут все простенько, можно и лучше но оно и так должно быстро работать и ТЗ не нарушать
    if (v.size() < 2) return;

    for (size_t i = 0; i + 1 < v.size(); ++i) {
        block_swap_parallel(
            v[i], v[v.size() - 1],
            block_length,
            threadCount,
            MIN_PARALLEL_LENGTH,
            pool
        );
    }
}

template <typename ForwardIt>
std::pair<ForwardIt, ForwardIt> left_rotate_aliquot(//работает только если length_a > length_b
    ForwardIt it_a, ForwardIt it_b,
    size_t length_a, size_t length_b,
    size_t threadCount, size_t MIN_PARALLEL_LENGTH,
    boost::asio::thread_pool &pool
) {
    assert(length_a > length_b);
    using BlockIt = ForwardIt;

    size_t block_count = length_a / length_b + 1; //кол-во полных блоков (не считая начало)
    size_t remainder = length_a % length_b; //остаток
    size_t block_size = length_b; //размер одного блока

    ForwardIt remainder_it = it_a;
    std::advance(it_a, remainder);//оставляем остаток и перекидываем it_a на начало правой части
    ForwardIt rotated_it = it_a;

    //минимально должно обрабатываться по 2 блока в потоке
    size_t min_block_chunk_count = block_count / 2;
    //учитываем, что сами блоки разной длины
    size_t maxThreads = std::max<size_t>(min_block_chunk_count, block_count / threadCount);
    //ограничиваем число потоков
    size_t current_thread_count = std::min(threadCount, maxThreads);

    //размер 1 чанка в блоках
    size_t block_chunk_size = block_count / current_thread_count;
    size_t block_chunk_remainder = block_count % current_thread_count;

    //ветктор для свапнутых влево блоков в каждом потоке O(кол-во потоков) не O(N)
    std::vector<BlockIt> second_level;
    second_level.reserve(current_thread_count);

    std::latch done(current_thread_count);

    // заполняем текущий уровень
    for (size_t i = 0; i < current_thread_count; ++i) {
        size_t current_block_chunk_size = block_chunk_size + (i < block_chunk_remainder ? 1 : 0);
        if (current_block_chunk_size == 0) break;

        ForwardIt begin = it_a;
        ForwardIt last_block = it_a;
        std::advance(last_block, (current_block_chunk_size - 1) * block_size);

        ForwardIt blockStartA = it_a;
        ForwardIt blockStartB = last_block;

        boost::asio::post(pool, [blockStartA, blockStartB, current_block_chunk_size, block_size, &done]() mutable {
            for (size_t j = 0; j < current_block_chunk_size - 1; ++j) {
                block_swap(blockStartA, blockStartB, block_size);
                std::advance(blockStartA, block_size);
            }
            done.count_down();
        });

        // сдвигаем итераторы на следующий блок и записывает сдвинутый блок в след уровень
        second_level.push_back(it_a);
        std::advance(it_a, current_block_chunk_size * block_size);
    }

    //ждем пока все отработает
    done.wait();

    //тут second_level O(кол-во потоков), так что можно себе многое позволить
    rotate_first_block_backward_in_vector(
        second_level, 
        block_size,
        threadCount, 
        MIN_PARALLEL_LENGTH,
        pool
    );

    // Возврат диапазона (remainder_it слева, он первый, это корректно)
    return {remainder_it, rotated_it};
}
