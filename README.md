# nsqueue

A high-performance, lock-free queue library for C++ with pre-allocated memory, optimized for minimal enqueue/dequeue latency. Currently supports SPSC (single-producer/single-consumer) queues, with MPMC (multi-producer/multi-consumer) support planned for future releases.

## Features

- **Lock-free**: Wait-free operations using atomic operations
- **Cache-optimized**: Strategic alignment to prevent false sharing
- **Header-only**: Easy integration into your project
- **Memory efficient**: Automatic stack/heap allocation based on size
- **Template-based**: Compile-time optimization with generic types
- **C++20**: Modern C++ implementation

## Requirements

- C++20 or later
- CMake 3.22 or later (for building tests/benchmarks)

## Installation

Since nsqueue is a header-only library, simply include the header in your project:

```cpp
#include "spsc_queue.h"
```

Or use CMake to integrate it into your build system:

```cmake
add_subdirectory(nsqueue)
target_link_libraries(your_target PRIVATE nsqueue)
```

## Queue Types

### `nsqueue::spsc_queue<T, N>`

A **single-producer/single-consumer** lock-free queue optimized for minimizing enqueue/dequeue latency.

**Template Parameters:**
- `T`: The type of elements stored in the queue
- `N`: Queue capacity (must be a power of 2)

**Key Features:**
- Lock-free push/pop operations
- Cache line alignment to prevent false sharing
- Automatic stack allocation for small queues (≤512KB), heap for larger
- Wait-free operations with `force_push`/`force_pop` variants
- Batch consume operations

**API:**

```cpp
// Construction
spsc_queue<int, 1024> queue;

// Non-blocking operations (returns false if full/empty)
bool emplace(Args&&... args);
bool push(const T& item);
bool pop(T& item);
bool pop();

// Blocking operations (spin-waits until success)
void force_emplace(Args&&... args);
void force_push(const T& item);
void force_pop(T& item);
void force_pop();

// Batch consumption
template<typename F> bool consume_one(F&& func);
template<typename F> size_t consume_all(F&& func);
template<typename F> size_t consume_n(F&& func, size_t n);

// Query operations
bool empty() const;
bool full() const;
size_t size() const;
size_t capacity() const;
T& front();

// Utility
void reset();
```

**Example:**

```cpp
#include "spsc_queue.h"
#include <thread>

nsqueue::spsc_queue<int, 1024> queue;

// Producer thread
std::thread producer([&]() {
    for (int i = 0; i < 100; ++i) {
        queue.force_push(i);  // Blocks if queue is full
    }
});

// Consumer thread
std::thread consumer([&]() {
    queue.consume_all([](int value) {
        // Process each value
        std::cout << value << std::endl;
    });
});

producer.join();
consumer.join();
```

## Performance Characteristics

- **Enqueue/Dequeue**: O(1) amortized time complexity
- **Memory**: O(N) where N is the queue capacity
- **Cache efficiency**: Separate cache lines for reader/writer indices to prevent false sharing
- **Allocation**: Stack allocation for queues ≤512KB, heap allocation for larger queues

## Roadmap

- [x] SPSC (Single-Producer/Single-Consumer) queue
- [ ] MPMC (Multi-Producer/Multi-Consumer) queue support
- [ ] Additional synchronization primitives
- [ ] Performance benchmarks and comparisons

## Building Tests and Benchmarks

```bash
mkdir build && cd build
cmake .. -DNSQUEUE_BUILD_TESTS=ON -DNSQUEUE_BUILD_BENCHMARKS=ON
cmake --build .
ctest  # Run tests
```

## License

MIT License - see [LICENSE](LICENSE) for details

## Contributing

Contributions are welcome! Please feel free to submit issues and pull requests.
