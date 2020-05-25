/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#pragma once
#include <ply-runtime/Core.h>
#include <atomic>

namespace ply {

// clang-format off
inline void signalFenceConsume() { std::atomic_signal_fence(std::memory_order_acquire); }
inline void signalFenceAcquire() { std::atomic_signal_fence(std::memory_order_acquire); }
inline void signalFenceRelease() { std::atomic_signal_fence(std::memory_order_release); }
inline void signalFenceSeqCst() { std::atomic_signal_fence(std::memory_order_seq_cst); }

inline void threadFenceConsume() { std::atomic_thread_fence(std::memory_order_acquire); }
inline void threadFenceAcquire() { std::atomic_thread_fence(std::memory_order_acquire); }
inline void threadFenceRelease() { std::atomic_thread_fence(std::memory_order_release); }
inline void threadFenceSeqCst() { std::atomic_thread_fence(std::memory_order_seq_cst); }
// clang-format on

enum MemoryOrder {
    Relaxed = std::memory_order_relaxed,
    Consume = std::memory_order_consume,
    Acquire = std::memory_order_acquire,
    Release = std::memory_order_release,
    ConsumeRelease = std::memory_order_acq_rel,
    AcquireRelease = std::memory_order_acq_rel,
};

template <typename T>
class Atomic_CPP11 : protected std::atomic<T> {
public:
    Atomic_CPP11() {
    }
    Atomic_CPP11(T value) : std::atomic<T>{value} {
    }
    Atomic_CPP11(const Atomic_CPP11& other) : std::atomic<T>{other.loadNonatomic()} {
    }
    // Hide operator=
    void operator=(T value) = delete;
    void operator=(const Atomic_CPP11& other) {
        storeNonatomic(other.loadNonatomic());
    }
    T loadNonatomic() const {
        return std::atomic<T>::load(std::memory_order_relaxed);
    }
    T load(MemoryOrder memoryOrder) const {
        return std::atomic<T>::load((std::memory_order) memoryOrder);
    }
    void storeNonatomic(T value) {
        return std::atomic<T>::store(value, std::memory_order_relaxed);
    }
    void store(T value, MemoryOrder memoryOrder) {
        return std::atomic<T>::store(value, (std::memory_order) memoryOrder);
    }
    T compareExchange(T expected, T desired, MemoryOrder memoryOrder) {
        std::atomic<T>::compare_exchange_strong(expected, desired, (std::memory_order) memoryOrder);
        return expected; // modified by reference by compare_exchange_strong
    }
    bool compareExchangeStrong(T& expected, T desired, MemoryOrder memoryOrder) {
        return std::atomic<T>::compare_exchange_strong(expected, desired,
                                                       (std::memory_order) memoryOrder);
    }
    bool compareExchangeWeak(T& expected, T desired, MemoryOrder success, MemoryOrder failure) {
        return std::atomic<T>::compare_exchange_weak(expected, desired, (std::memory_order) success,
                                                     (std::memory_order) failure);
    }
    T exchange(T desired, MemoryOrder memoryOrder) {
        return std::atomic<T>::exchange(desired, (std::memory_order) memoryOrder);
    }
    T fetchAdd(T operand, MemoryOrder memoryOrder) {
        return std::atomic<T>::fetch_add(operand, (std::memory_order) memoryOrder);
    }
    T fetchSub(T operand, MemoryOrder memoryOrder) {
        return std::atomic<T>::fetch_sub(operand, (std::memory_order) memoryOrder);
    }
    T fetchAnd(T operand, MemoryOrder memoryOrder) {
        return std::atomic<T>::fetch_and(operand, (std::memory_order) memoryOrder);
    }
    T fetchOr(T operand, MemoryOrder memoryOrder) {
        return std::atomic<T>::fetch_or(operand, (std::memory_order) memoryOrder);
    }
};

} // namespace ply
