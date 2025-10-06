#pragma once

#include <random>
#include <chrono>

namespace mms {

// Seeded random number generator for reproducible simulations
class RNG {
public:
    explicit RNG(uint64_t seed = 0) : gen_(seed) {}
    
    // Generate uniform random integer in [min, max]
    template<typename T>
    T uniform_int(T min, T max) {
        std::uniform_int_distribution<T> dist(min, max);
        return dist(gen_);
    }
    
    // Generate uniform random float in [0, 1)
    double uniform_real() {
        std::uniform_real_distribution<double> dist(0.0, 1.0);
        return dist(gen_);
    }
    
    // Generate uniform random float in [min, max)
    double uniform_real(double min, double max) {
        std::uniform_real_distribution<double> dist(min, max);
        return dist(gen_);
    }
    
    // Generate exponential random variable (for Poisson processes)
    double exponential(double lambda) {
        std::exponential_distribution<double> dist(lambda);
        return dist(gen_);
    }
    
    // Generate normal random variable
    double normal(double mean, double stddev) {
        std::normal_distribution<double> dist(mean, stddev);
        return dist(gen_);
    }
    
    // Generate Poisson random variable
    int poisson(double lambda) {
        std::poisson_distribution<int> dist(lambda);
        return dist(gen_);
    }
    
    // Generate geometric random variable (for discrete events)
    int geometric(double p) {
        std::geometric_distribution<int> dist(p);
        return dist(gen_);
    }
    
    // Reset with new seed
    void seed(uint64_t new_seed) {
        gen_.seed(new_seed);
    }
    
    // Get current seed (for debugging)
    uint64_t get_seed() const {
        return gen_.default_seed;
    }
    
    // Generate random boolean
    bool bernoulli(double p = 0.5) {
        std::bernoulli_distribution dist(p);
        return dist(gen_);
    }
    
    // Choose random element from container
    template<typename Container>
    auto choose(const Container& container) -> decltype(*container.begin()) {
        if (container.empty()) {
            throw std::runtime_error("Cannot choose from empty container");
        }
        auto it = container.begin();
        std::advance(it, uniform_int<size_t>(0, container.size() - 1));
        return *it;
    }
    
    // Shuffle container in place
    template<typename Container>
    void shuffle(Container& container) {
        std::shuffle(container.begin(), container.end(), gen_);
    }

private:
    std::mt19937_64 gen_;
};

// Utility function to generate seed from current time
inline uint64_t generate_time_seed() {
    auto now = std::chrono::high_resolution_clock::now();
    auto duration = now.time_since_epoch();
    return std::chrono::duration_cast<std::chrono::nanoseconds>(duration).count();
}

} // namespace mms
