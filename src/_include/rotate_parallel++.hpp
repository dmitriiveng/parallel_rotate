#pragma once

#include <forward_list>
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
#include <limits>
#include <chrono>
#include <boost/asio.hpp>
#include <semaphore>

template <typename ForwardIt>
std::pair<ForwardIt, ForwardIt> rotate_aliquot_forward_inplace(
    ForwardIt it_a, ForwardIt it_b,
    size_t length_a, size_t length_b,
    boost::asio::thread_pool &pool
) {
    using BlockIt = ForwardIt;
    bool cond = length_a < length_b;

    size_t block_count = cond? (length_b / length_a) : (length_a / length_b); // количество блоков R
    size_t remainder = cond? (length_b % length_a) : (length_a % length_b);
    size_t block_size = cond? length_a : length_b;
    std::vector<BlockIt> it_blocks; // вектор блоков правой части
    it_blocks.reserve(block_count + 1);

    // Заполняем вектор блоков
    if(length_a < length_b) {
        ForwardIt it = it_a;
        it_blocks.push_back(it);
        it = it_b;
        for (size_t i = 1; i < block_count + 1; ++i) {
            it_blocks.push_back(it);
            std::advance(it, block_size);
        }
    }
    else {
        ForwardIt it = it_a;
        std::advance(it, remainder);
        for (size_t i = 1; i < block_count + 1; ++i) {
            it_blocks.push_back(it);
            std::advance(it, block_size);
        }
        it = it_b;
        it_blocks.push_back(it);
    }

    // Начинаем пирамидально свапать по уровням
    std::vector<BlockIt> current_level = it_blocks; // текущий уровень правых блоков
    std::vector<BlockIt> next_level;

    while (current_level.size() > 1) {
        size_t pairs = current_level.size() / 2;
        next_level.clear();
        std::counting_semaphore<> level_sem(0);

        for (size_t i = 0; i < pairs * 2; i += 2) {
            BlockIt left = current_level[i];
            BlockIt right = current_level[i + 1];

            boost::asio::post(pool, [left, right, block_size, &level_sem]() {
                ForwardIt a = left;
                ForwardIt b = right;
                for (size_t j = 0; j < block_size; ++j, ++a, ++b) {
                    std::iter_swap(a, b);
                }
                level_sem.release();
            });
        }

        // Ждем, пока все свапы текущего уровня завершатся
        for (size_t i = 0; i < pairs; ++i) {
            level_sem.acquire();
        }

        // Формируем следующий уровень: берем правый элемент каждой пары
        for (size_t i = 0; i < pairs * 2; i += 2) {
            if(cond) next_level.push_back(current_level[i + 1]);
            else next_level.push_back(current_level[i]);
        }

        // Если остался один элемент (нечетный), переносим его на следующий уровень
        if (current_level.size() % 2 == 1) {
            next_level.push_back(current_level.back());
        }

        current_level.swap(next_level);
    }

    if(cond) {
        BlockIt new_it_b = current_level[0];
        std::advance(new_it_b, block_size);
        return {current_level[0], new_it_b};
    }
    else{
        return {it_a, current_level[0]};
    }
}

template <typename ForwardIt>
ForwardIt rotate_forward_inplace_low_middle(ForwardIt first, ForwardIt middle, ForwardIt last, size_t threadCount) {
    if (first == middle || middle == last) return first;

    boost::asio::thread_pool pool(threadCount);

    size_t leftLen = std::distance(first, middle);
    size_t rightLen = std::distance(middle, last);

    ForwardIt leftBegin = first;
    ForwardIt rightBegin = middle;

    while (leftLen > 0 && rightLen > 0) {

        std::pair<ForwardIt, ForwardIt> rv_iterators = 
            rotate_aliquot_forward_inplace(leftBegin, rightBegin, leftLen, rightLen, pool);
        
        leftBegin = rv_iterators.first;
        rightBegin = rv_iterators.second;

        if(leftLen < rightLen) rightLen %= leftLen;
        else leftLen %= rightLen;
    }

    pool.join();
    return leftBegin;
}