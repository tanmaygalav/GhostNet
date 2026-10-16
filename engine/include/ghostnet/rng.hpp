#pragma once

#include <random>
#include <cstdint>

namespace ghostnet {

class DeterministicRNG {
public:
    explicit DeterministicRNG(uint64_t seed = 42)
        : initial_seed_(seed), engine_(seed), uniform_dist_(0.0, 1.0) {}

    void reseed(uint64_t seed) {
        initial_seed_ = seed;
        engine_.seed(seed);
    }

    [[nodiscard]] uint64_t initial_seed() const noexcept {
        return initial_seed_;
    }

    // Generates a uniform random float in [0.0, 1.0)
    double uniform_01() {
        return uniform_dist_(engine_);
    }

    // Evaluates packet loss decision based on configured loss probability [0.0, 1.0]
    bool evaluate_loss(double probability) {
        if (probability <= 0.0) return false;
        if (probability >= 1.0) return true;
        return uniform_01() < probability;
    }

    // Uniform integer in [min, max]
    uint64_t uniform_int(uint64_t min, uint64_t max) {
        if (min >= max) return min;
        std::uniform_int_distribution<uint64_t> dist(min, max);
        return dist(engine_);
    }

    // Exponential distribution interval for Poisson arrivals (in microseconds)
    // rate_per_second: average arrivals per second
    uint64_t exponential_interval_us(double rate_per_second) {
        if (rate_per_second <= 0.0) return 0;
        std::exponential_distribution<double> dist(rate_per_second);
        double seconds = dist(engine_);
        return static_cast<uint64_t>(seconds * 1'000'000.0);
    }

private:
    uint64_t initial_seed_;
    std::mt19937_64 engine_;
    std::uniform_real_distribution<double> uniform_dist_;
};

} // namespace ghostnet