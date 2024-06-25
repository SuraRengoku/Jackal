//
// Created by jonas on 2024/6/24.
//

#ifndef JPARALLELWRAPPER_H
#define JPARALLELWRAPPER_H

#include <vector>
#include <algorithm>
#include <functional>

#include "tbb/spin_mutex.h"
#include "tbb/parallel_for.h" //Intel Threading buidling blocks
#include "tbb/concurrent_vector.h"

namespace JackalRenderer{
    enum class JExecutionPolicy { J_SERIAL, J_PARALLEL };

    /**
     * @brief  fills from begin to end with value in parallel
     * @param begin
     * @param end
     * @param value
     * @param policy
     */
    template<typename RandomIterator, typename T>
    void parallelFill(const RandomIterator &begin, const RandomIterator &end, const T &value, JExecutionPolicy policy = JExecutionPolicy::J_PARALLEL);

    /**
     * @brief makes a for-loop from beginIndex to endIndex in parallel
     * @param beginIndex
     * @param endIndex
     * @param function
     * @param policy
     */
    template<typename IndexType, typename Function>
    void parallelLoop(IndexType beginIndex, IndexType endIndex, const Function &function, JExecutionPolicy policy = JExecutionPolicy::J_PARALLEL) {
        if(beginIndex > endIndex) return;
        // Refs: https://blog.csdn.net/kezunhai/article/details/44678845
        if(policy == JExecutionPolicy::J_PARALLEL) tbb::parallel_for(beginIndex, endIndex, function);
        else {
            for(auto i = beginIndex; i < endIndex; ++i)
                function(i);
        }
    }

    /**
     * @brief makes a for-loop from beginIndex to endIndex in parallel with given chunk size
     * @param beginIndex
     * @param endIndex
     * @param grainSize
     * @param function
     * @param policy
     */
    template<typename Function>
    void parallelLoopWithAffinity(size_t beginIndex, size_t endIndex, const Function &function, JExecutionPolicy policy = JExecutionPolicy::J_PARALLEL) {
        if(beginIndex > endIndex) return;
        static tbb::affinity_partitioner ap;
        if(policy == JExecutionPolicy::J_PARALLEL) tbb::parallel_for(beginIndex, endIndex, function, ap);
        else {
            for(auto i = beginIndex; i < endIndex; ++i)
                function(i);
        }
    }

    template<typename RandomIterator, typename T>
    void parallelFill(const RandomIterator &begin, const RandomIterator &end, const T &value, JExecutionPolicy policy) {
        auto diff = end - begin;
        if(diff <= 0) return;
        size_t size = static_cast<size_t>(diff);
        parallelLoop(0, size, [begin, value](size_t i){ begin[i] = value;}, policy);
    }

}


#endif //JPARALLELWRAPPER_H
