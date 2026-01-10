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

template <typename ForwardIt>
std::pair<ForwardIt, ForwardIt> block_swap(ForwardIt it_a, ForwardIt it_b, size_t length){
    for (size_t i = 0; i < length; ++i, ++it_a, ++it_b) {
        std::iter_swap(it_a, it_b);
    }
    return std::pair<ForwardIt, ForwardIt>(it_a, it_b);
}

template <typename ForwardIt>
std::pair<ForwardIt, ForwardIt> block_swap_parallel(
    ForwardIt a, ForwardIt b, 
    size_t length, 
    size_t threadCount, size_t MIN_PARALLEL_LENGTH,
    boost::asio::thread_pool &pool) {
    if (length == 0) return std::pair<ForwardIt, ForwardIt>{a, b};

    //оптимизация, переход на однопоточку при малых length, работать и без него будет но медленнее
    if (length <= MIN_PARALLEL_LENGTH || threadCount == 1) {
        ForwardIt itA = a;
        ForwardIt itB = b;
        for (size_t i = 0; i < length; ++i, ++itA, ++itB) {
            std::iter_swap(itA, itB);
        }
        std::advance(a, length);
        std::advance(b, length);
        return {a, b};
    }

    //ограничиваем число потоков
    size_t maxThreads = std::max<size_t>(1, length / MIN_PARALLEL_LENGTH);
    threadCount = std::min(threadCount, maxThreads);

    size_t baseSize = length / threadCount;
    size_t remainder = length % threadCount;

    ForwardIt aIt = a;
    ForwardIt bIt = b;

    std::latch done(threadCount);

    for (size_t i = 0; i < threadCount; ++i) {
        size_t blockSize = baseSize + (i < remainder ? 1 : 0);
        if (blockSize == 0) break;

        ForwardIt blockA = aIt;
        ForwardIt blockB = bIt;

        boost::asio::post(pool, [blockA, blockB, blockSize, &done]() mutable {
            ForwardIt itA = blockA;
            ForwardIt itB = blockB;
            for (size_t j = 0; j < blockSize; ++j, ++itA, ++itB) {
                std::iter_swap(itA, itB);
            }
            done.count_down();
        });

        // сдвигаем итераторы на следующий блок (то, что за нас делал бы spliterator)
        for (size_t j = 0; j < blockSize; ++j) { ++aIt; ++bIt; }
    }

    done.wait();

    return std::pair<ForwardIt, ForwardIt>{aIt, bIt}; // итераторы на end свапнутых областей
}