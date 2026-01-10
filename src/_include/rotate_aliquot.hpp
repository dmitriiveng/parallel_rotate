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

#include "block_swap.hpp"
#include "right_rotate_aliquot.hpp"
#include "left_rotate_aliqout.hpp"


template <typename ForwardIt>
std::pair<ForwardIt, ForwardIt> rotate_aliquot(
    ForwardIt it_a, ForwardIt it_b,
    size_t length_a, size_t length_b,
    size_t threadCount, size_t MIN_PARALLEL_LENGTH,
    boost::asio::thread_pool &pool
) {
    assert(length_a!=length_b);
    if(length_a < length_b){
        return right_rotate_aliquot(
            it_a, it_b,
            length_a, length_b,
            threadCount, MIN_PARALLEL_LENGTH,
            pool
        );
    }
    else{
        return left_rotate_aliquot(
            it_a, it_b,
            length_a, length_b,
            threadCount, MIN_PARALLEL_LENGTH,
            pool
        );
    }
}