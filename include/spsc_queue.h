#include <array>
#include <atomic>
#include <bitset>
#include <cstddef>
#include <memory>
#include <stdexcept>
#include <utility>

constexpr std::size_t STACK_BYTES = 524'288;

namespace nsqueue {

namespace details {

#ifdef _cpp_lib_hardware_interference_size
static constexpr std::size_t cacheLineSize = std::hardware_destructive_interference_size;
#else
static constexpr std::size_t cacheLineSize = 64;
#endif

}  // namespace details

template <typename T, std::size_t N>
class spsc_queue {
    static_assert((N & (N - 1)) == 0, "N must be a power of two");
    static constexpr std::size_t storage_bytes = 2 * N * details::cacheLineSize;
    static constexpr bool        use_heap      = storage_bytes > STACK_BYTES;

public:
    using index_t = std::size_t;

    spsc_queue()                                   = default;
    spsc_queue(const spsc_queue& other)            = delete;
    spsc_queue& operator=(const spsc_queue& other) = delete;
    spsc_queue(spsc_queue&& other)                 = delete;
    spsc_queue& operator=(spsc_queue&& other)      = delete;
    ~spsc_queue()                                  = default;

    template <typename... Args>
    [[nodiscard]] bool emplace(Args&&... args) noexcept {
        auto writeIdx     = writer_.writeIndex_.load(std::memory_order_relaxed);
        auto nextWriteIdx = (writeIdx + 1) & mask_;

        if (nextWriteIdx == writer_.readIndexCache_) [[unlikely]] {
            writer_.readIndexCache_ = reader_.readIndex_.load(std::memory_order_acquire);
            if (nextWriteIdx == writer_.readIndexCache_) [[unlikely]]
                return false;
        }

        new (&items_[writeIdx].mObj) T(std::forward<Args>(args)...);
        writer_.writeIndex_.store(nextWriteIdx, std::memory_order_relaxed);

        return true;
    }

    template <typename... Args>
    void force_emplace(Args&&... args) noexcept {
        auto writeIdx     = writer_.writeIndex_.load(std::memory_order_relaxed);
        auto nextWriteIdx = (writeIdx + 1) & mask_;

        while (nextWriteIdx == writer_.readIndexCache_) {
            writer_.readIndexCache_ = reader_.readIndex_.load(std::memory_order_acquire);
        }

        new (&items_[writeIdx].mObj) T(std::forward<Args>(args)...);
        writer_.writeIndex_.store(nextWriteIdx, std::memory_order_relaxed);
    }

    [[nodiscard]] bool push(T const& item) noexcept { return emplace(item); }

    void force_push(T const& item) noexcept { return force_emplace(item); }

    void force_pop(T& item) noexcept {
        auto readIdx  = reader_.readIndex_.load(std::memory_order_relaxed);
        auto writeIdx = reader_.writeIndexCache_;
        while (readIdx == writeIdx) {
            writeIdx = reader_.writeIndexCache_
                = writer_.writeIndex_.load(std::memory_order_acquire);
        }

        item = std::move(items_[readIdx].mObj);

        auto nextReadIdx = (readIdx + 1) & mask_;
        reader_.readIndex_.store(nextReadIdx, std::memory_order_relaxed);
    }

    void force_pop() noexcept {
        auto readIdx  = reader_.readIndex_.load(std::memory_order_relaxed);
        auto writeIdx = reader_.writeIndexCache_;
        while (readIdx == writeIdx) {
            writeIdx = reader_.writeIndexCache_
                = writer_.writeIndex_.load(std::memory_order_acquire);
        }

        auto nextReadIdx = (readIdx + 1) & mask_;
        reader_.readIndex_.store(nextReadIdx, std::memory_order_relaxed);
    }

    [[nodiscard]] bool pop(T& item) noexcept {
        auto readIdx  = reader_.readIndex_.load(std::memory_order_relaxed);
        auto writeIdx = reader_.writeIndexCache_;
        if (readIdx == writeIdx) [[unlikely]] {
            writeIdx = reader_.writeIndexCache_
                = writer_.writeIndex_.load(std::memory_order_acquire);
            if (readIdx == writeIdx) [[unlikely]]
                return false;
        }

        item = std::move(items_[readIdx].mObj);

        auto nextReadIdx = (readIdx + 1) & mask_;
        reader_.readIndex_.store(nextReadIdx, std::memory_order_relaxed);

        return true;
    }

    bool pop() noexcept {
        auto readIdx  = reader_.readIndex_.load(std::memory_order_relaxed);
        auto writeIdx = reader_.writeIndexCache_;
        if (readIdx == writeIdx) [[unlikely]] {
            writeIdx = reader_.writeIndexCache_
                = writer_.writeIndex_.load(std::memory_order_acquire);
            if (readIdx == writeIdx) [[unlikely]]
                return false;
        }

        auto nextReadIdx = (readIdx + 1) & mask_;
        reader_.readIndex_.store(nextReadIdx, std::memory_order_relaxed);

        return true;
    }

    template <typename F>
    bool consume_one(F&& func) noexcept {
        auto readIdx  = reader_.readIndex_.load(std::memory_order_relaxed);
        auto writeIdx = reader_.writeIndexCache_;
        if (readIdx == writeIdx) [[unlikely]] {
            writeIdx = reader_.writeIndexCache_
                = writer_.writeIndex_.load(std::memory_order_acquire);
            if (readIdx == writeIdx) [[unlikely]]
                return false;
        }

        func(std::move(items_[readIdx].mObj));

        auto nextReadIdx = (readIdx + 1) & mask_;
        reader_.readIndex_.store(nextReadIdx, std::memory_order_release);

        return true;
    }

    template <typename F>
    index_t consume_all(F&& func) noexcept {
        index_t n{0};
        while (consume_one(std::forward<F>(func)))
            ++n;
        return n;
    }

    template <typename F>
    index_t consume_n(F&& func, index_t n) noexcept {
        index_t m{};
        for (; m < n; ++m) {
            if (!consume_one(std::forward<F>(func)))
                break;
        }
        return m;
    }

    [[nodiscard]] bool full() const noexcept {
        auto writeIdx     = writer_.writeIndex_.load(std::memory_order_acquire);
        auto nextWriteIdx = (writeIdx + 1) & mask_;
        auto readIdx      = reader_.readIndex_.load(std::memory_order_acquire);
        return nextWriteIdx == readIdx;
    }

    [[nodiscard]] index_t size() const noexcept {
        auto w = writer_.writeIndex_.load(std::memory_order_acquire);
        auto r = reader_.readIndex_.load(std::memory_order_acquire);
        return (w >= r) ? w - r : (N - r) + w;
    }

    [[nodiscard]] bool empty() const noexcept {
        return writer_.writeIndex_.load(std::memory_order_acquire)
            == reader_.readIndex_.load(std::memory_order_acquire);
    }

    [[nodiscard]] index_t read_available() const noexcept { return size(); }

    [[nodiscard]] T& front() noexcept {
        const auto r = reader_.readIdx_.load(std::memory_order_acquire);
        return items_[r].mObj;
    }

    [[nodiscard]] T const& front() const noexcept {
        const auto r = reader_.readIdx_.load(std::memory_order_acquire);
        return items_[r].mObj;
    }

    [[nodiscard]] size_t capacity() const noexcept { return mask_; }

    void reset(void) noexcept {
        writer_.readIndexCache_  = 0;
        reader_.writeIndexCache_ = 0;
        reader_.readIndex_.store(0, std::memory_order_relaxed);
        writer_.writeIndex_.store(0, std::memory_order_relaxed);
    }

private:
    struct AlignedData {
        alignas(details::cacheLineSize * 2) T mObj{};
    };
    static constexpr index_t mask_{N - 1};

    template <typename U, std::size_t SIZE, bool heap>
    struct queue_storage;

    template <typename U, std::size_t SIZE>
    struct queue_storage<U, SIZE, true> {
        std::array<U, SIZE> data;
        U&                  operator[](index_t idx) noexcept { return data[idx]; }
        const U&            operator[](index_t idx) const noexcept { return data[idx]; }
    };
    template <typename U, std::size_t SIZE>
    struct queue_storage<U, SIZE, false> {
        std::unique_ptr<U[]> data;
        queue_storage()
            : data(std::make_unique<U[]>(SIZE)) {}
        U&       operator[](index_t idx) noexcept { return data[idx]; }
        const U& operator[](index_t idx) const noexcept { return data[idx]; }
    };
    queue_storage<AlignedData, N, use_heap> items_;

    struct alignas(details::cacheLineSize) ReadState {
        std::atomic<index_t> readIndex_{0};
        index_t              writeIndexCache_{0};
    } reader_;
    struct alignas(details::cacheLineSize) WriteState {
        std::atomic<index_t> writeIndex_{0};
        index_t              readIndexCache_{0};
    } writer_;
};

}  // namespace nsqueue
