#include <atomic>
#include <boost/lockfree/spsc_queue.hpp>
#include <cassert>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <immintrin.h>
#include <iostream>
#include <mutex>
#include <nanobench.h>
#include <numeric>
#include <pthread.h>
#include <queue>
#include <thread>

#include "deaod/spsc_queue.h"
#include "dro/spsc_queue.h"
#include "moodycamel/spsc_queue.h"
#include "mutex/spsc_queue.h"
#include "spsc_queue.h"

#define CONSUMER_CPU 1
#define PRODUCER_CPU 3

constexpr std::size_t N        = 1'000'000;
constexpr std::size_t CAPACITY = 1 << 12;

inline void pinThread(std::thread& t, int cpu) {
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(cpu, &cpuset);
    int rc = pthread_setaffinity_np(t.native_handle(), sizeof(cpu_set_t), &cpuset);
    if (rc != 0)
        throw std::runtime_error("Failed to pin thread");
}

inline void pinThread(int cpu) {
    if (cpu < 0)
        return;
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(cpu, &cpuset);
    if (pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset) == -1) {
        perror("pthread_setaffinity_no");
        exit(1);
    }
}

template <typename T>
void bench_force(T& buffer) {
    std::atomic<bool> ready{false};
    std::thread       consumer = std::thread([&] {
        pinThread(CONSUMER_CPU);
        while (!ready.load(std::memory_order_acquire))
            continue;
        for (uint64_t i{}; i < N; ++i) {
            uint64_t val;
            buffer.force_pop(val);
            if (val != i) {
                throw std::runtime_error("wrong ordering");
            }
        }
    });

    pinThread(PRODUCER_CPU);

    ready.store(true, std::memory_order_release);

    for (uint64_t i{}; i < N; ++i) {
        buffer.force_push(i);
    }
    consumer.join();
}

template <typename T>
void bench_force_dro(T& buffer) {
    std::atomic<bool> ready{false};
    std::thread       consumer = std::thread([&] {
        pinThread(CONSUMER_CPU);
        while (!ready.load(std::memory_order_acquire))
            continue;
        for (uint64_t i{}; i < N; ++i) {
            uint64_t val;
            buffer.pop(val);
            if (val != i) {
                throw std::runtime_error("wrong ordering");
            }
        }
    });

    pinThread(PRODUCER_CPU);

    ready.store(true, std::memory_order_release);

    for (uint64_t i{}; i < N; ++i) {
        buffer.push(i);
    }
    consumer.join();
}
template <typename T>
void bench_force_moodycamel(T& buffer) {
    std::atomic<bool> ready{false};
    std::thread       consumer = std::thread([&] {
        pinThread(CONSUMER_CPU);
        while (!ready.load(std::memory_order_acquire))
            continue;
        for (uint64_t i{}; i < N; ++i) {
            uint64_t val;
            buffer.wait_dequeue(val);
            if (val != i) {
                throw std::runtime_error("wrong ordering");
            }
        }
    });

    pinThread(PRODUCER_CPU);

    ready.store(true, std::memory_order_release);

    for (uint64_t i{}; i < N; ++i) {
        buffer.wait_enqueue(i);
    }
    consumer.join();
}

template <typename T>
void bench_try(T& buffer) {
    std::atomic<bool> ready{false};
    std::thread       consumer = std::thread([&] {
        pinThread(CONSUMER_CPU);
        while (!ready.load(std::memory_order_acquire))
            continue;
        for (uint64_t i{}; i < N; ++i) {
            uint64_t val;
            while (!buffer.pop(val))
                continue;
            if (val != i) {
                throw std::runtime_error("wrong ordering");
            }
        }
    });

    pinThread(PRODUCER_CPU);

    ready.store(true, std::memory_order_release);

    for (uint64_t i{}; i < N; ++i) {
        while (!buffer.push(i))
            continue;
    }
    consumer.join();
}

template <typename T>
void bench_try_dro(T& buffer) {
    std::atomic<bool> ready{false};
    std::thread       consumer = std::thread([&] {
        pinThread(CONSUMER_CPU);
        while (!ready.load(std::memory_order_acquire))
            continue;
        for (uint64_t i{}; i < N; ++i) {
            uint64_t val;
            while (!buffer.try_pop(val))
                continue;
            if (val != i) {
                throw std::runtime_error("wrong ordering");
            }
        }
    });

    pinThread(PRODUCER_CPU);

    ready.store(true, std::memory_order_release);

    for (uint64_t i{}; i < N; ++i) {
        while (!buffer.try_push(i))
            continue;
    }
    consumer.join();
}

template <typename T>
void bench_try_moodycamel(T& buffer) {
    std::atomic<bool> ready{false};
    std::thread       consumer = std::thread([&] {
        pinThread(CONSUMER_CPU);
        while (!ready.load(std::memory_order_acquire))
            continue;
        for (uint64_t i{}; i < N; ++i) {
            uint64_t val;
            while (!buffer.try_dequeue(val))
                continue;
            if (val != i) {
                throw std::runtime_error("wrong ordering");
            }
        }
    });

    pinThread(PRODUCER_CPU);

    ready.store(true, std::memory_order_release);

    for (uint64_t i{}; i < N; ++i) {
        while (!buffer.try_enqueue(i))
            continue;
    }
    consumer.join();
}

int main() {

    using object = uint64_t;

    mutex_queue<object>                                    mutex_queue_(CAPACITY);
    boost::lockfree::spsc_queue<object>                    boost_queue_(CAPACITY);
    deaod::spsc_queue<object, CAPACITY>                    deaod_queue_;
    dro::SPSCQueue<object, CAPACITY - 1>                   dro_queue_;
    moodycamel::BlockingReaderWriterCircularBuffer<object> moodycamel_queue_(CAPACITY);
    nsqueue::spsc_queue<object, CAPACITY>                  nsqueue_;

    ankerl::nanobench::Bench bench;
    bench.warmup(10).epochs(100).minEpochIterations(10).performanceCounters(true);

    // bench.run("boost", [&] { bench_try(boost_queue_); });
    // bench.run("deaod", [&] { bench_try(deaod_queue_); });
    bench.run("dro", [&] { bench_force_dro(dro_queue_); });
    // bench.run("moodycamel", [&] { bench_force_moodycamel(moodycamel_queue_); });
    bench.run("nsqueue", [&] { bench_force(nsqueue_); });

    return 0;
}
