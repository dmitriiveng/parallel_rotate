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

#include "rotate_aliquot.hpp"

template <typename ForwardIt>
ForwardIt rotate_forward_inplace(ForwardIt first, ForwardIt middle, ForwardIt last, size_t threadCount) {
    if (first == middle || middle == last) return first;

    constexpr size_t MIN_PARALLEL_LENGTH = 1000;
    boost::asio::thread_pool pool(threadCount);

    size_t leftLen = std::distance(first, middle);
    size_t rightLen = std::distance(middle, last);

    ForwardIt leftBegin = first;
    ForwardIt rightBegin = middle;

    while (leftLen > 0 && rightLen > 0) {
        size_t block_count = leftLen < rightLen ? (rightLen / leftLen) + 1 : (leftLen / rightLen) + 1;
        size_t block_size = leftLen < rightLen ? leftLen : rightLen;

        if(block_count / 2 >= threadCount && block_size < MIN_PARALLEL_LENGTH){
            std::pair<ForwardIt, ForwardIt> rv_iterators = 
                rotate_aliquot(leftBegin, rightBegin, leftLen, rightLen, threadCount, MIN_PARALLEL_LENGTH, pool);
            
            leftBegin = rv_iterators.first;
            rightBegin = rv_iterators.second;

            if(leftLen < rightLen) rightLen %= leftLen;
            else leftLen %= rightLen;
        }
        else{
            if (leftLen <= rightLen) {
                // свапаем левый блок с первой частью правого блока той же длины
                std::pair<ForwardIt, ForwardIt> rv_end_iterators = 
                    block_swap_parallel(leftBegin, rightBegin, leftLen, threadCount, MIN_PARALLEL_LENGTH, pool);
                // после свапа левый блок сдвинулся вправо
                leftBegin = rightBegin;
                rightBegin = rv_end_iterators.second;
                rightLen -= leftLen;
            } else {
                // левый блок больше, свапаем правый блок с началом левого блока той же длинны
                std::pair<ForwardIt, ForwardIt> rv_end_iterators = 
                    block_swap_parallel(leftBegin, rightBegin, rightLen, threadCount, MIN_PARALLEL_LENGTH, pool);
                // теперь нужно обновить размер области слева и 
                // поменять leftBegin, который соответствует rv_end_iterators.first
                leftLen -= rightLen;
                leftBegin = rv_end_iterators.first;
            }
        }
    }

    pool.join();
    return leftBegin;
}
