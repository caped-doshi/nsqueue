template <class T>
class mutex_queue {
public:
    mutex_queue(std::size_t capacity)
        : capacity_{capacity} {}
    bool push(T item) {
        {
            std::lock_guard<std::mutex> lock(m_);
            q_.push(item);
        }
        cv_.notify_one();
        return true;
    }
    template <typename F>
    bool consume_one(F&& func) {
        std::unique_lock<std::mutex> lock(m_);
        cv_.wait(lock, [&] { return !q_.empty(); });
        auto item = q_.front();
        (std::forward<F>(func))(item);
        q_.pop();
        return true;
    }
    bool pop(T& item) {
        std::unique_lock<std::mutex> lock(m_);
        cv_.wait(lock, [&] { return !q_.empty(); });
        item = q_.front();
        q_.pop();
        return true;
    }
    bool pop() {
        std::unique_lock<std::mutex> lock(m_);
        cv_.wait(lock, [&] { return !q_.empty(); });
        q_.pop();
        return true;
    }

private:
    std::size_t             capacity_;
    std::queue<T>           q_;
    std::mutex              m_;
    std::condition_variable cv_;
};