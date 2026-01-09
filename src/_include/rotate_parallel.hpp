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
    size_t threadCount,
    boost::asio::thread_pool &pool) {
    using BlockIt = ForwardIt;
    bool cond = length_a < length_b;

    size_t block_count = cond ? (length_b / length_a) + 1 : (length_a / length_b) + 1; //кол-во полных блоков (не считая начало)
    size_t remainder = cond ? (length_b % length_a) : (length_a % length_b); //остаток
    size_t block_size = cond ? length_a : length_b; //размер одного блока

    //тут я собираю массив forward iterator`ов, по сути тут не O(1), но и не O(N), 
    //размер зависит от положения middle 
    //с учетом уточнения ТЗ "считайте что у вас spliterator и вы уже чудесным образом знаете начало каждого куска", 
    //вроде как ТЗ не нарушается, потому что в теории, до начала алгоритма можно было бы вычислить все 
    //это рекурсивно сплитератором.
    std::vector<BlockIt> current_level;
    current_level.reserve(block_count);

    // заполняем текущий уровень
    if (cond) {
        current_level.push_back(it_a);
        ForwardIt it = it_b;
        for (size_t i = 0; i < block_count - 1; ++i) {
            current_level.push_back(it);
            std::advance(it, block_size);
        }
    } else {
        ForwardIt it = it_a;
        std::advance(it, remainder);
        for (size_t i = 0; i < block_count - 1; ++i) {
            current_level.push_back(it);
            std::advance(it, block_size);
        }
        current_level.push_back(it_b);
    }

    while (current_level.size() > 1) {
        size_t pairs = current_level.size() / 2;
        std::counting_semaphore<> level_sem(0);

        // Выполняем свапы
        for (size_t i = 0; i < pairs; ++i) {
            BlockIt left = current_level[i * 2];
            BlockIt right = current_level[i * 2 + 1];

            boost::asio::post(pool, [left, right, block_size, &level_sem]() {
                ForwardIt a = left;
                ForwardIt b = right;
                for (size_t j = 0; j < block_size; ++j, ++a, ++b) {
                    std::iter_swap(a, b);
                }
                level_sem.release();
            });
        }

        for (size_t i = 0; i < pairs; ++i) {
            level_sem.acquire();
        }

        // Переносим правые элементы на «следующую итерацию»
        size_t write_idx = 0;
        for (size_t i = 0; i < pairs; ++i) {
            current_level[write_idx++] = cond ? current_level[i * 2 + 1] : current_level[i * 2];
        }

        // Если остался один элемент (нечетный), переносим его на следующий уровень
        if (current_level.size() % 2 == 1) {
            current_level[write_idx++] = current_level.back();
        }

        current_level.resize(write_idx);
    }

    // Возврат диапазона
    if (cond) {
        BlockIt new_it_b = current_level[0];
        std::advance(new_it_b, block_size);
        return {current_level[0], new_it_b};
    } else {
        return {it_a, current_level[0]};
    }
}

template <typename ForwardIt>
std::pair<ForwardIt, ForwardIt> swap_blocks_parallel(
    ForwardIt a, ForwardIt b, 
    size_t length, 
    size_t threadCount, 
    boost::asio::thread_pool &pool) {
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

    //work_guard, чтобы пул не завершился раньше времени
    std::counting_semaphore<> sem(0);

    for (size_t i = 0; i < threadCount; ++i) {
        size_t blockSize = baseSize + (i < remainder ? 1 : 0);
        if (blockSize == 0) break;

        ForwardIt blockA = aIt;
        ForwardIt blockB = bIt;

        boost::asio::post(pool, [blockA, blockB, blockSize, &sem]() mutable {
            ForwardIt itA = blockA;
            ForwardIt itB = blockB;
            for (size_t j = 0; j < blockSize; ++j, ++itA, ++itB) {
                std::iter_swap(itA, itB);
            }
            sem.release();
        });

        // сдвигаем итераторы на следующий блок (то, что за нас делал бы spliterator)
        for (size_t j = 0; j < blockSize; ++j) { ++aIt; ++bIt; }
    }

    for (size_t i = 0; i < threadCount; ++i) {
        sem.acquire(); // уменьшаем счётчик, блокируется, если задачи не завершены
    } // вся эта система с счетчиками <semaphore> нужна чтобы предотвратить возможный дата рейс между вызовами ф-ции

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
        size_t block_count = leftLen < rightLen ? (leftLen / rightLen) : (rightLen / leftLen);
        size_t block_size = leftLen < rightLen ? leftLen : rightLen;
        if(block_count >= threadCount ){//тут надо написать более правильное условие
            std::pair<ForwardIt, ForwardIt> rv_iterators = 
                rotate_aliquot_forward_inplace(leftBegin, rightBegin, leftLen, rightLen, threadCount, pool);
            
            leftBegin = rv_iterators.first;
            rightBegin = rv_iterators.second;

            if(leftLen < rightLen) rightLen %= leftLen;
            else leftLen %= rightLen;
        }
        else{
            if (leftLen <= rightLen) {
                // свапаем левый блок с первой частью правого блока той же длины
                std::pair<ForwardIt, ForwardIt> rv_end_iterators = 
                    swap_blocks_parallel(leftBegin, rightBegin, leftLen, threadCount, pool);
                // после свапа левый блок сдвинулся вправо
                leftBegin = rightBegin;
                rightBegin = rv_end_iterators.second;
                rightLen -= leftLen;
            } else {
                // левый блок больше, свапаем правый блок с началом левого блока той же длинны
                std::pair<ForwardIt, ForwardIt> rv_end_iterators = 
                    swap_blocks_parallel(leftBegin, rightBegin, rightLen, threadCount, pool);
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
            rotate_aliquot_forward_inplace(leftBegin, rightBegin, leftLen, rightLen, threadCount, pool);
        
        leftBegin = rv_iterators.first;
        rightBegin = rv_iterators.second;

        if(leftLen < rightLen) rightLen %= leftLen;
        else leftLen %= rightLen;
    }

    pool.join();
    return leftBegin;
}


template <typename ForwardIt>
ForwardIt rotate_forward_inplace_swap_blocks(ForwardIt first, ForwardIt middle, ForwardIt last, size_t threadCount) {
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
            // левый блок больше, свапаем правый блок с началом левого блока той же длинны
            std::pair<ForwardIt, ForwardIt> rv_end_iterators = 
                swap_blocks_parallel(leftBegin, rightBegin, rightLen, threadCount, pool);

            // теперь нужно обновить размер области слева и 
            // поменять leftBegin, который соответствует rv_end_iterators.first
            leftLen -= rightLen;
            leftBegin = rv_end_iterators.first;
        }
    }

    pool.join();
    return leftBegin;
}