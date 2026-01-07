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

#include "rotate_parallel.hpp"

template <typename ForwardIt>
std::pair<ForwardIt, ForwardIt> swap_blocks_parallel(ForwardIt a, ForwardIt b, size_t length, size_t threadCount, boost::asio::thread_pool &pool) {
    if (length == 0) return std::pair<ForwardIt, ForwardIt>{a, b};

    //оптимизация, переход на однопоточку при малых length, работать и без него будет но медленнее
    constexpr size_t MIN_PARALLEL_LENGTH = 1000;
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
    }

    return std::pair<ForwardIt, ForwardIt>{aIt, bIt}; // итераторы на end свапнутых областей
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