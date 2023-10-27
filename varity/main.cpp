#include "lru.h"
#include <experimental/random>
#include <thread>
#include <vector>


template<>
std::string LruCache<std::string, std::string>::prepare_reply(const std::string& req) const {
    return "Reply for request " + req + ": some data";
}


std::string random_request(int max) {
    int num = std::experimental::randint(1, max);
    return "req_" + std::to_string(num);
}


int main(int argc, const char** argv) {

    LruCache<std::string, std::string> lru(10);

    std::vector<std::thread> threads;
    for (int i = 0; i < 10; ++i) {
        threads.emplace_back([&lru]() {
            for (int j = 0; j < 100; ++j) {
                //std::cout << lru.make_request(random_request(3)) << std::endl;
                lru.make_request(random_request(20));
            }
        });
    };

    for (auto& t : threads) {
        t.join();
    }

    return 0;
}