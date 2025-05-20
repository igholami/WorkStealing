//
// Created by Iman Gholami on 5/20/25.
//

#ifndef ABPSCHEDULER_WORKSTEALINGDEQUE_H
#define ABPSCHEDULER_WORKSTEALINGDEQUE_H

#include <atomic>
#include <functional>

/**
 * Lock‑free work‑stealing deque
 */
template<typename T, size_t CapacityPow2 = 65536>
class WorkStealingDeque {
    static_assert((CapacityPow2 & (CapacityPow2 - 1)) == 0,
                  "Capacity must be a power of two");
public:
    WorkStealingDeque() : top_(0), bottom_(0) {
        for (auto &slot: buffer_) slot.store(nullptr, std::memory_order_relaxed);
    }

    /**
     * Owner pushes on the bottom — wait‑free.
     */
    bool push_bottom(T *value) {
        size_t b = bottom_.load(std::memory_order_relaxed);
        size_t t = top_.load(std::memory_order_acquire);
        if (b - t >= CapacityPow2) {
            return false;                 // deque full
        }
        buffer_[b & (CapacityPow2 - 1)].store(value, std::memory_order_relaxed);
        std::atomic_thread_fence(std::memory_order_release);
        bottom_.store(b + 1, std::memory_order_relaxed);
        return true;
    }

    /**
     * Owner pops from the bottom — wait‑free.
     */
    std::optional<T *> pop_bottom() {
        size_t b = bottom_.load(std::memory_order_relaxed);
        if (b == 0) return std::nullopt;  // empty
        b -= 1;
        bottom_.store(b, std::memory_order_relaxed);
        std::atomic_thread_fence(std::memory_order_seq_cst);
        size_t t = top_.load(std::memory_order_relaxed);
        std::optional<T *> result;
        if (t <= b) {
            result = buffer_[b & (CapacityPow2 - 1)].load(std::memory_order_relaxed);
            if (t == b) {
                // last element — need to swing both indices to empty state
                if (!top_.compare_exchange_strong(t, t + 1,
                                                  std::memory_order_release,
                                                  std::memory_order_relaxed)) {
                    // lost a race with a thief
                    result.reset();
                }
                bottom_.store(b + 1, std::memory_order_relaxed);
            }
        } else {
            // Deque looked empty — restore bottom
            bottom_.store(b + 1, std::memory_order_relaxed);
        }
        return result;
    }

    /**
     * Thief steals from the top — lock‑free.
     */
    std::optional<T *> steal_top() {
        size_t t = top_.load(std::memory_order_acquire);
        std::atomic_thread_fence(std::memory_order_seq_cst);
        size_t b = bottom_.load(std::memory_order_acquire);
        if (t >= b) return std::nullopt;  // empty
        T *value = buffer_[t & (CapacityPow2 - 1)].load(std::memory_order_relaxed);
        if (top_.compare_exchange_strong(t, t + 1,
                                         std::memory_order_release,
                                         std::memory_order_relaxed)) {
            return value;                 // success!
        }
        return std::nullopt;              // lost race, caller can retry
    }

private:
    std::array<std::atomic<T *>, CapacityPow2> buffer_;
    std::atomic<size_t> top_;
    std::atomic<size_t> bottom_;
};

#endif //ABPSCHEDULER_WORKSTEALINGDEQUE_H
