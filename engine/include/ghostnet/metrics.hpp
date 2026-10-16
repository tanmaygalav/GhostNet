#pragma once

#include "ghostnet/event.hpp"
#include <vector>
#include <cstdint>
#include <algorithm>
#include <numeric>

namespace ghostnet {

struct MetricsSummary {
    uint64_t total_generated{0};
    uint64_t total_completed{0};
    uint64_t total_failed{0};
    uint64_t dropped_by_loss{0};
    uint64_t timeouts{0};
    uint64_t queue_overflows{0};
    uint64_t service_down_failures{0};

    double completion_rate{0.0};
    double mean_latency_us{0.0};
    SimTime min_latency_us{0};
    SimTime p50_latency_us{0};
    SimTime p95_latency_us{0};
    SimTime p99_latency_us{0};
    SimTime max_latency_us{0};
};

class MetricsAccumulator {
public:
    MetricsAccumulator() = default;

    void record_generated() noexcept { ++total_generated_; }
    void record_loss_drop() noexcept { ++dropped_by_loss_; ++total_failed_; }
    void record_timeout() noexcept { ++timeouts_; ++total_failed_; }
    void record_queue_overflow() noexcept { ++queue_overflows_; ++total_failed_; }
    void record_service_down() noexcept { ++service_down_failures_; ++total_failed_; }

    void record_completion(SimTime latency) {
        ++total_completed_;
        latency_samples_us_.push_back(latency);
    }

    [[nodiscard]] MetricsSummary summarize() const {
        MetricsSummary summary;
        summary.total_generated = total_generated_;
        summary.total_completed = total_completed_;
        summary.total_failed = total_failed_;
        summary.dropped_by_loss = dropped_by_loss_;
        summary.timeouts = timeouts_;
        summary.queue_overflows = queue_overflows_;
        summary.service_down_failures = service_down_failures_;

        if (total_generated_ > 0) {
            summary.completion_rate = static_cast<double>(total_completed_) / static_cast<double>(total_generated_);
        }

        if (latency_samples_us_.empty()) {
            return summary;
        }

        std::vector<SimTime> sorted_samples = latency_samples_us_;
        std::sort(sorted_samples.begin(), sorted_samples.end());

        summary.min_latency_us = sorted_samples.front();
        summary.max_latency_us = sorted_samples.back();

        double sum = std::accumulate(sorted_samples.begin(), sorted_samples.end(), 0.0);
        summary.mean_latency_us = sum / static_cast<double>(sorted_samples.size());

        auto get_percentile = [&](double p) -> SimTime {
            if (sorted_samples.empty()) return 0;
            size_t idx = static_cast<size_t>(p * static_cast<double>(sorted_samples.size() - 1));
            return sorted_samples[idx];
        };

        summary.p50_latency_us = get_percentile(0.50);
        summary.p95_latency_us = get_percentile(0.95);
        summary.p99_latency_us = get_percentile(0.99);

        return summary;
    }

    void reset() {
        total_generated_ = 0;
        total_completed_ = 0;
        total_failed_ = 0;
        dropped_by_loss_ = 0;
        timeouts_ = 0;
        queue_overflows_ = 0;
        service_down_failures_ = 0;
        latency_samples_us_.clear();
    }

private:
    uint64_t total_generated_{0};
    uint64_t total_completed_{0};
    uint64_t total_failed_{0};
    uint64_t dropped_by_loss_{0};
    uint64_t timeouts_{0};
    uint64_t queue_overflows_{0};
    uint64_t service_down_failures_{0};
    std::vector<SimTime> latency_samples_us_{};
};

} // namespace ghostnet