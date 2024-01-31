#pragma once
#include <random>
#include <iterator>

namespace Random {
    extern std::mt19937 gen;

    template<typename T>
    T get(T low, T high) {
        std::uniform_real_distribution<> distr(low, high);
        return distr(gen);
    }

    template<typename Iterable>
    auto get(Iterable& collection) -> decltype(std::begin(collection)) {
        using difference_type = typename std::iterator_traits<decltype(std::begin(collection))>::difference_type;
        std::uniform_int_distribution<difference_type> distr(0, std::distance(std::begin(collection), std::end(collection)) - 1);
        auto random_it = std::begin(collection);
        std::advance(random_it, distr(gen)); // Move the iterator to a random position
        return random_it;
    }

}