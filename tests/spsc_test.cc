#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_all.hpp>

#include <thread>
#include <atomic>
#include <vector>

#include "spsc_queue.h"

TEST_CASE("push/pop","[unit]") {
    nsqueue::spsc_queue<int,8> q;
    REQUIRE(q.empty());
    REQUIRE(q.size() == 0);

    for(int i{1}; i<=3; ++i)
        REQUIRE(q.emplace(i));

    REQUIRE(q.empty() == false);
    REQUIRE(q.size() == 3);

    int val{};
    for(int i{1}; i<= 3; ++i){
        REQUIRE(q.pop(val));
        REQUIRE(val == i);
    }
    
    REQUIRE(q.empty());
    REQUIRE(q.pop(val) == false);
};

TEST_CASE("consume_one", "[unit]"){
    nsqueue::spsc_queue<int, 8> q;
    
    REQUIRE_FALSE(q.consume_one([](int){}));

    REQUIRE(q.emplace(10));
    REQUIRE(q.emplace(20));

    int sum{0};
    REQUIRE(q.consume_one([&](int v){sum += v;}));
    REQUIRE(sum == 10);

    REQUIRE(q.consume_one([&](int v){sum += v;}));
    REQUIRE(sum == 30);

    REQUIRE(q.empty());
    REQUIRE_FALSE(q.consume_one([](int){}));
};

TEST_CASE("consume_all","[unit]"){
    nsqueue::spsc_queue<int, 8> q;
    for(int i{0};i<5;++i)
        REQUIRE(q.emplace(i));

    int sum{0};
    auto n = q.consume_all([&](int v) {sum += v;});

    REQUIRE(n==5);
    REQUIRE(sum == n*(n-1)/2);
    REQUIRE(q.empty());
};

TEST_CASE("fail on full queue", "[unit]"){
    nsqueue::spsc_queue<int, 8> q;
    for(int i{1}; i<= 7; ++i)
        REQUIRE(q.emplace(i));
    
    REQUIRE(q.full());
    REQUIRE_FALSE(q.emplace(8));
};

TEST_CASE("reset", "[unit]"){
    nsqueue::spsc_queue<int, 8> q;

    REQUIRE(q.emplace(1)); 
    REQUIRE(q.emplace(2));
    
    q.reset();

    REQUIRE(q.empty());
    REQUIRE(q.emplace(3));
    int val{};
    REQUIRE(q.pop(val));
    REQUIRE(val == 3);
};

TEST_CASE("move-only type", "[unit]"){
    nsqueue::spsc_queue<std::unique_ptr<int>, 8> q;

    REQUIRE(q.emplace(std::make_unique<int>(42)));
    
    std::unique_ptr<int> p;
    REQUIRE(q.pop(p));
    
    REQUIRE(*p == 42);
};

TEST_CASE("wrap around", "[unit]"){
    nsqueue::spsc_queue<int, 4> q;
    
    for(int i{1}; i<= 3;++i)
        REQUIRE(q.emplace(i));
    REQUIRE_FALSE(q.emplace(4));
    
    int val{};
    REQUIRE(q.pop(val)); REQUIRE(val == 1);
    REQUIRE(q.emplace(4));

    for(int i{2}; i<=4; ++i){
        REQUIRE(q.pop(val)); REQUIRE(val == i);
    }

    REQUIRE(q.empty());
};

TEST_CASE("stress", "[stress]"){
    constexpr int N = 200'000;
    nsqueue::spsc_queue<int,1024> q;

    std::atomic<bool> done{false};

    std::thread producer([&] {
        for(int i{0}; i<N; ++i){
            while(!q.emplace(i)) { continue; }
        }
        done.store(true, std::memory_order_release);
    });

    std::thread consumer([&] {
        int expected{0}; int val;
        while(!done.load(std::memory_order_acquire) || !q.empty()){
            if(q.pop(val)){
                if(val != expected)
                    FAIL("out of order");
                ++expected;
            }
        }
        REQUIRE(expected== N);
    });
    
    producer.join();
    consumer.join();
};
